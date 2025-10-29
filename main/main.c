#include "sdkconfig.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "driver/i2c_master.h"
#include "LCD1602.h"

#define I2C_MASTER_SCL_IO           22       /*!< GPIO number used for I2C master clock */
#define I2C_MASTER_SDA_IO           21       /*!< GPIO number used for I2C master data  */
#define I2C_MASTER_NUM              I2C_NUM_0                   /*!< I2C port number for master dev */
#define I2C_MASTER_FREQ_HZ          100000 /*!< I2C master clock frequency */
#define I2C_MASTER_TX_BUF_DISABLE   0                           /*!< I2C master doesn't need buffer */
#define I2C_MASTER_RX_BUF_DISABLE   0                           /*!< I2C master doesn't need buffer */
static const char *TAG = "example";

/**
 * @brief i2c master initialization
 */
static void i2c_master_init(i2c_master_bus_handle_t *bus_handle);

void app_main(void)
{
    vTaskDelay(3000 / portTICK_PERIOD_MS);
    i2c_master_bus_handle_t bus_handle;
    i2c_master_init(&bus_handle);
    ESP_LOGI(TAG, "I2C initialized successfully");
    
    LCD1602 mylcd;
    LCDinit(&mylcd, LCD_I2C_ADDR, I2C_MASTER_FREQ_HZ, &bus_handle);
    ESP_LOGI(TAG, "LCD1602 initialized successfully");
    
    LCDsetBackgroundLight(&mylcd, BackgroundLightON);
    LCDclear(&mylcd);
    LCDsetCursor(&mylcd, 0, 0);
    LCDprint(&mylcd, "Hola Mundo!");
    LCDsetCursor(&mylcd, 0, 1);
    LCDprint(&mylcd, "ESP32 LCD");
    
    ESP_LOGI(TAG, "Mensaje enviado al LCD");
    
    vTaskDelay(2000 / portTICK_PERIOD_MS);
    
    ESP_ERROR_CHECK(i2c_master_bus_rm_device(mylcd.i2c_handler));
    ESP_ERROR_CHECK(i2c_del_master_bus(bus_handle));
    ESP_LOGI(TAG, "I2C de-initialized successfully");
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