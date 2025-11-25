#include "cJSON.h"
#include "esp_adc/adc_cali.h"
#include "esp_err.h"
#include "esp_rom_sys.h"
#include "hal/adc_types.h"
#include "hal/ledc_types.h"
#include "lwip/sockets.h"
#include "sdkconfig.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/FreeRTOSConfig_arch.h"
#include "freertos/projdefs.h"
#include "freertos/queue.h"
#include "esp_log.h"
#include "driver/i2c_master.h"
#include "nvs_flash.h"
#include "soc/gpio_num.h"
#include "LCD1602.h"
#include "AM2302.h"
#include "ADC.h"
#include "WiFi.h"
#include "PWM.h"
#include "zeroCross.h"
#include "PIDControl.h"
#include <stdbool.h>
#include <stdint.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>

#define I2C_MASTER_SCL_IO           22
#define I2C_MASTER_SDA_IO           21
#define I2C_MASTER_NUM              I2C_NUM_0
#define I2C_MASTER_FREQ_HZ          100000
#define I2C_MASTER_TX_BUF_DISABLE   0
#define I2C_MASTER_RX_BUF_DISABLE   0

#define PRIORITY_0                  0
#define PRIORITY_1                  1
#define PRIORITY_2                  2

#define SSID CONFIG_SSID
#define PSSWD CONFIG_PSSWD
#define SERVER_IP CONFIG_SERVER_IP
#define SERVER_PORT CONFIG_SERVER_PORT

#define IRRIGATION_PIN 17

static const char *TAG = "Main app";

/**
 * @brief i2c master initialization
 */
static void i2c_master_init(i2c_master_bus_handle_t *bus_handle);

/**
 * @brief      Prints "T:   °C  H:  %" into the lcd
 *
 * @param      lcd   Where the text will apear
 */
static void printStaticCharsLCD(LCD1602 *lcd);

/**
 * @brief      Task for updating temperature and humidity displayed in LCD approximately every 2 seconds
 *
 */
void updateLCDContent(void *pvParameters);

/**
 * @brief      Task for reading AM2302 approximately every 2 seconds
 *
 */
void readAM2302(void *pvParameters);

/**
 * @brief      Task for reading LM135 approximately every 2 seconds
 *
 * @param      pvParameters  The pv parameters
 */
void readLM135(void *pvParameters);

/**
 * @brief      Task that sends sensors data to server
 *
 */
void sendDataToServer(void *pvParameters);

/**
 * @brief      Task for receiving functions to execute from server
 *
 */
void receiveFunctionExecutionFromServer(void *pvParameters);


/**
 * @brief      Executes function with funcName with argument arg
 *
 * @param[in]  funcName  Function name
 * @param[in]  arg       Argument
 * @warning Only works for certain functions
 */
void executeFunction(const char funcName[], const float arg);

/**
 * @brief      Task for execute PID control
 *
 */
void PIDControl(void *pvParameters);

/**
 * Global variables
 */
int TCPSocket;
LCD1602 informationLCD;
AM2302Handler am2302;
ADCHandler ADC_U1;
LM135Handler lm135;
FanHandler coolerFan;
PIDController BulbPowerPIDController;
bool irrigationLevel = false;


void app_main(void){      
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
      ESP_ERROR_CHECK(nvs_flash_erase());
      ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    esp_err_t WiFiStatus = WiFiInit(SSID, PSSWD);
    if(WIFI_SUCCESS != WiFiStatus){
        ESP_LOGE(TAG, "Failed to associate to AP, dying ...");
        return;
    }
    TCPSocket = socket(AF_INET, SOCK_STREAM, 0);
    if(TCPSocket < 0){
        ESP_LOGE(TAG, "Failed to create socket");
        return;
    } 
    if(TCP_FAILURE == connectTCPServer(TCPSocket, SERVER_IP, htons(SERVER_PORT))){
        close(TCPSocket);
        return;
    }
    int flags = fcntl(TCPSocket, F_GETFL, 0);
    fcntl(TCPSocket, F_SETFL, flags | O_NONBLOCK);
    xTaskCreate(sendDataToServer, "TCP Connection", 6144, NULL, PRIORITY_2, NULL);
    xTaskCreate(receiveFunctionExecutionFromServer, "Instructions", 6144, NULL, PRIORITY_2, NULL);
    
    esp_err_t AM2302status = AM2302init(&am2302, GPIO_NUM_23);
    if(ESP_OK == AM2302status){
        ESP_LOGI(TAG, "AM2302 initialized successfully");
        xTaskCreate(readAM2302, "A2302", 4096, NULL, PRIORITY_1, NULL);
    }

    esp_err_t ADC1Status = ADCconfigUnitBasic(&ADC_U1, ADC_UNIT_1);
    ADC1Status += ADCconfigChannel(&ADC_U1, ADC_ATTEN_DB_12, ADC_BITWIDTH_12, ADC_CHANNEL_4);
    if(ESP_OK == ADC1Status && ESP_OK == LM135init(&lm135, &ADC_U1)){
        ESP_LOGI(TAG, "LM135 initialized successfully");
        xTaskCreate(readLM135, "LM135", 3072, NULL, PRIORITY_1, NULL);
    }
    
    if(FanInit(&coolerFan, GPIO_NUM_19, LEDC_CHANNEL_0) != ESP_OK){
        ESP_LOGE(TAG, "Cannot initialize cooler fan PWM");
    }
    
    setPIDDesiredValue(&BulbPowerPIDController, 0.0);
    setPIDGains(&BulbPowerPIDController, 0.8, 0.005, 0.001);
    setPIDMaxAndMinVals(&BulbPowerPIDController, MIN_BUBL_POWER, MAX_BULB_POWER);
    esp_err_t ZXStatus = zeroCrossInit();
    if(ESP_OK ==  ZXStatus)
        xTaskCreate(PIDControl, "PID control", 3072, NULL, PRIORITY_1, NULL);

    esp_err_t irrigationStatus = gpio_reset_pin(IRRIGATION_PIN);
    if(irrigationStatus){
        ESP_LOGE(TAG, "Invalid GPIO pin for irrigation");
    }
    else{
        gpio_set_direction(IRRIGATION_PIN, GPIO_MODE_OUTPUT);
        gpio_set_level(IRRIGATION_PIN, irrigationLevel);
    }

    i2c_master_bus_handle_t bus_handle;
    i2c_master_init(&bus_handle);
    esp_err_t LCDStatus = LCDinit(&informationLCD, LCD_I2C_ADDR, I2C_MASTER_FREQ_HZ, &bus_handle);
    if(ESP_OK == LCDStatus){
        ESP_LOGI(TAG, "LCD initialized successfully");
        LCDsetBackgroundLight(&informationLCD, BackgroundLightON);
        printStaticCharsLCD(&informationLCD);
        xTaskCreate(updateLCDContent, "LCD", 4096, NULL, PRIORITY_0, NULL);   
    }
}


void readAM2302(void *pvParameters){
    while (true) {
        AM2302read(&am2302);
        vTaskDelay(pdMS_TO_TICKS(2000));
    }
}

void readLM135(void *pvParameters){
    while (true) {
        LM135read(&lm135);
        vTaskDelay(pdMS_TO_TICKS(2000));
    }
}


void sendDataToServer(void *pvParameters){
    esp_err_t Tst;
    while(true){
        Tst = sendSensorsDataToServer(TCPSocket, lm135.temperature, am2302.humidity, am2302.temperature);
        if(Tst == TCP_FAILURE){
            ESP_LOGE(TAG, "Connection with server lost");
            close(TCPSocket);
            break;
        }
        vTaskDelay(pdMS_TO_TICKS(2000));
    }
    vTaskDelete(NULL);
}

void receiveFunctionExecutionFromServer(void *pvParameters){
    char rxBuffer[128];
    ssize_t len;
    char funcName[64];
    float arg;
    while (true){
        len = recv(TCPSocket, rxBuffer, sizeof(rxBuffer) - 1, MSG_DONTWAIT);
        if( len == -1 && (errno == EAGAIN || errno == EWOULDBLOCK)){
            // No data received from server
            vTaskDelay(pdMS_TO_TICKS(500)); 
        }
        else if (len > 0) {
            // Data received
            rxBuffer[len] = '\0';
            decodeJSONServerMessage(rxBuffer, funcName, &arg);
            executeFunction(funcName, arg);
        }
        else if (len == 0) {
            ESP_LOGE(TAG, "Connection closed by peer");
            break;
        }
    }
    vTaskDelete(NULL);
}

void executeFunction(const char funcName[], const float arg){
    if(NULL == funcName){
        ESP_LOGE(TAG, "No se obtuvo nombre de funcion");
        return;
    }
    if(0 == strcmp(funcName, "toggleIrrigation")){
        irrigationLevel = !irrigationLevel;
        gpio_set_level(IRRIGATION_PIN, irrigationLevel);
        ESP_LOGI(TAG, "Toggle de sistema de irrigacion");
    }
    else if(0 == strcmp(funcName, "setDesiredTemperature")){
        setPIDDesiredValue(&BulbPowerPIDController, arg);
        ESP_LOGI(TAG, "Temperatura ajustada: %.3f", arg);
    }
    else if(0 == strcmp(funcName, "setFanPower")){
        setFanDutyCyclePerc(&coolerFan, arg);
        ESP_LOGI(TAG, "Modificacion de potencia de ventilador: %f", arg);
    }
    else{
        ESP_LOGE(TAG, "Funcion no reconocida: %s", funcName);
    }
}

void PIDControl(void *pvParameters){
    float power;
    while (true) {
        power = computePIDOutput(&BulbPowerPIDController, am2302.temperature);
        setBulbPowerPerc(power);
        vTaskDelay(pdMS_TO_TICKS(250));
    }
}


void updateLCDContent(void *pvParameters){
    while (true) {
        LCDsetCursor(&informationLCD, 9, 0);
        LCDprint(&informationLCD, "%02.1f", am2302.temperature);
        LCDsetCursor(&informationLCD, 9, 1);
        LCDprint(&informationLCD,"%02.1f",  am2302.humidity);
        vTaskDelay(pdMS_TO_TICKS(3000));
    }
}

static void i2c_master_init(i2c_master_bus_handle_t *bus_handle)
{
    i2c_master_bus_config_t bus_config = {
        .i2c_port = I2C_MASTER_NUM,
        .sda_io_num = I2C_MASTER_SDA_IO,
        .scl_io_num = I2C_MASTER_SCL_IO,
        .clk_source = I2C_CLK_SRC_DEFAULT,
        .glitch_ignore_cnt = 7,
        .flags.enable_internal_pullup = true,
    };
    ESP_ERROR_CHECK(i2c_new_master_bus(&bus_config, bus_handle));
}

static void printStaticCharsLCD(LCD1602 *_lcd){
    LCDsetCursor(_lcd, 0, 0);
    LCDprint(_lcd, "Tempture:");
    LCDsetCursor(_lcd, 13, 0);
    LCDprintCelsiusSymbol(_lcd);
    LCDsetCursor(_lcd, 0, 1);
    LCDprint(_lcd, "Humidity:");
    LCDsetCursor(_lcd, 13, 1);
    LCDprintPercentageSymbol(_lcd);  
}
