#include "cJSON.h"
#include "esp_adc/adc_cali.h"
#include "esp_err.h"
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
 * @brief      Sends a data to server approximately every 2 seconds
 *
 */
void sendDataToServer(void *pvParameters);

/**
 * @brief      Receive instructions from server every second
 *
 */
void receiveInstructionsFromServer(void *pvParameters);


void decodeServerMessage(char buffer[]);


/**
 * Global variables
 */
int mySocket;
LCD1602 lcd;
AM2302Handler am2302;
ADCHandler ADC_U1;
LM135Handler lm135;
FanHandler coolerFan;


void app_main(void){    
    i2c_master_bus_handle_t bus_handle;
    i2c_master_init(&bus_handle);
    esp_err_t LCDStatus = LCDinit(&lcd, LCD_I2C_ADDR, I2C_MASTER_FREQ_HZ, &bus_handle);
    if(ESP_OK == LCDStatus){
        ESP_LOGI(TAG, "LCD initialized successfully");
        LCDsetBackgroundLight(&lcd, BackgroundLightON);
        //LCDclear(&lcd);
        printStaticCharsLCD(&lcd);
        xTaskCreate(updateLCDContent, "LCD", 4096, NULL, PRIORITY_0, NULL);   
    }

    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
      ESP_ERROR_CHECK(nvs_flash_erase());
      ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    esp_err_t WiFiStatus = WiFiInit();
    if(WIFI_SUCCESS != WiFiStatus){
        ESP_LOGE(TAG, "Failed to associate to AP, dying ...");
        return;
    }
    mySocket = socket(AF_INET, SOCK_STREAM, 0);
    if(mySocket < 0){
        ESP_LOGE(TAG, "Failed to create socket");
        return;
    } 
    if(TCP_FAILURE == connectTCPServer(mySocket)){
        close(mySocket);
        return;
    }
    int flags = fcntl(mySocket, F_GETFL, 0);
    fcntl(mySocket, F_SETFL, flags | O_NONBLOCK);
    xTaskCreate(sendDataToServer, "TCP Connection", 6144, NULL, PRIORITY_2, NULL);
    xTaskCreate(receiveInstructionsFromServer, "Instructions", 6144, NULL, PRIORITY_2, NULL);
    
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
    

}


void readAM2302(void *pvParameters){
    while (true) {
        AM2302read(&am2302);
        //ESP_LOGI(TAG, "Temperatura AM2302: %.2f", am2302.temperature);
        //ESP_LOGI(TAG, "Humedad AM2302: %.2f", am2302.humidity);
        vTaskDelay(pdMS_TO_TICKS(2000));
    }
}

void readLM135(void *pvParameters){
    while (true) {
        LM135read(&lm135);
        //ESP_LOGI(TAG, "Temperatura LM135: %.2f", lm135.temperature);
        vTaskDelay(pdMS_TO_TICKS(2000));
    }
}


void sendDataToServer(void *pvParameters){
    esp_err_t Tst;
    while(true){
        Tst = sendSensorsDataToServer(mySocket, lm135.temperature, am2302.humidity, am2302.temperature);
        if(Tst == TCP_FAILURE){
            ESP_LOGE(TAG, "Connection with server lost");
            close(mySocket);
            break;
        }
        vTaskDelay(pdMS_TO_TICKS(2000));
    }
    vTaskDelete(NULL);
}

void receiveInstructionsFromServer(void *pvParameters){
    char rxBuffer[128];
    ssize_t len;
    while (true){
        len = recv(mySocket, rxBuffer, sizeof(rxBuffer) - 1, MSG_DONTWAIT);
        if( len == -1 && (errno == EAGAIN || errno == EWOULDBLOCK)){
            // Sin datos recibidos por el servidor
            vTaskDelay(pdMS_TO_TICKS(100)); 
        }
        else if (len > 0) {
            // Recibio datos de servidor
            rxBuffer[len] = '\0';
            decodeServerMessage(rxBuffer);
        }
        else if (len == 0) {
            ESP_LOGE(TAG, "Connection closed by peer");
            break;
        }
    }
    vTaskDelete(NULL);
}

// TODO: Pasar esto al paquete de WiFi
void decodeServerMessage(char buffer[]){
    cJSON *json = cJSON_Parse(buffer);
    if (json == NULL) {
        const char *error_ptr = cJSON_GetErrorPtr();
        if (error_ptr != NULL) {
            ESP_LOGE(TAG, "Error: %s\n", error_ptr);
        }
        cJSON_Delete(json);
        return;
    }
    // TODO: Pasar esto a una funcion
    cJSON *func = cJSON_GetObjectItemCaseSensitive(json, "function");
    if(!cJSON_IsString(func) && func->valuestring == NULL){
        const char *error_ptr = cJSON_GetErrorPtr();
        if (error_ptr != NULL) {
            ESP_LOGE(TAG, "Error: %s\n", error_ptr);
        }
        cJSON_Delete(json);
        return;
    }
    // Funcion toggle no requiere argumentos
    if(0 == strcmp(func->valuestring, "toggleIrrigation")){
        // TODO: Poner aqui lo de toggle el sistema de irrigacion
        ESP_LOGI(TAG, "Modificacion de irrigacion");
        cJSON_Delete(json);
        return;
    }

    cJSON *arg = cJSON_GetObjectItemCaseSensitive(json, "argument");
    if(!cJSON_IsNumber(arg)){
        const char *error_ptr = cJSON_GetErrorPtr();
        if (error_ptr != NULL) {
            ESP_LOGE(TAG, "Error: %s\n", error_ptr);
        }
        cJSON_Delete(json);
        return;
    }
    if(0 == strcmp(func->valuestring, "setDesiredTemperature")){
        //TODO: Poner aqui lo de modificar temperatura
        ESP_LOGI(TAG, "Temperatura deseada: %d", arg->valueint);
    }
    else if(0 == strcmp(func->valuestring, "setFanPower")){
        setFanDutyCyclePerc(&coolerFan, (float)arg->valuedouble);
        ESP_LOGI(TAG, "Modificacion de potencia de ventilador a %f", (float)arg->valuedouble);
    }
    cJSON_Delete(json);
}


void updateLCDContent(void *pvParameters){
    while (true) {
        LCDsetCursor(&lcd, 9, 0);
        LCDprint(&lcd, "%02.1f", am2302.temperature);
        LCDsetCursor(&lcd, 9, 1);
        LCDprint(&lcd,"%02.1f",  am2302.humidity);
        vTaskDelay(pdMS_TO_TICKS(5000));
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
