#include "LCD1602.h"
#include "freertos/idf_additions.h"
#include "freertos/projdefs.h"
#include <stdint.h>

void LCDpulseEnable(LCD1602 *lcd, uint8_t data)
{
    uint8_t buffer;
    
    // Set enable bit high
    buffer = data | LCD_EN_BIT;
    ESP_ERROR_CHECK(i2c_master_transmit(lcd->i2c_handler, &buffer, 1, I2C_MASTER_TIMEOUT_MS / portTICK_PERIOD_MS));
    vTaskDelay(pdMS_TO_TICKS(1));
    
    // Set enable bit low
    buffer = data & ~LCD_EN_BIT;
    ESP_ERROR_CHECK(i2c_master_transmit(lcd->i2c_handler, &buffer, 1, I2C_MASTER_TIMEOUT_MS / portTICK_PERIOD_MS));
    vTaskDelay(pdMS_TO_TICKS(1));
}

void LCDsendByte(LCD1602 *lcd, uint8_t Byte, LCDsendMode mode)
{
    if (!lcd->isConnected) 
        return;

    uint8_t high_nibble, low_nibble;
    uint8_t flags = 0x00; // RW = 0 (write)
    
    if (mode == sendAsData) {
        flags |= LCD_RS_BIT; // RS = 1 para datos
    }
    // RS = 0 para comandos
    
    if (lcd->BackgroundLight == BackgroundLightON) {
        flags |= LCD_BL_BIT;
    }
    
    high_nibble = (Byte & 0xF0) | flags;
    LCDpulseEnable(lcd, high_nibble);
    
    low_nibble = ((Byte & 0x0F) << 4) | flags;
    LCDpulseEnable(lcd, low_nibble);
}

void LCDinit(LCD1602 *lcd, uint8_t i2c_addr, int i2c_speed,i2c_master_bus_handle_t *bus_handle)
{
    lcd->i2c_config.dev_addr_length = I2C_ADDR_BIT_LEN_7;
    lcd->i2c_config.device_address = i2c_addr;
    lcd->i2c_config.scl_speed_hz = i2c_speed;
    ESP_ERROR_CHECK(i2c_master_bus_add_device(*bus_handle, &lcd->i2c_config, &lcd->i2c_handler));
    lcd->isConnected = true;
    lcd->BackgroundLight = BackgroundLightON;
    
    vTaskDelay(pdMS_TO_TICKS(100));
    
    LCDsendByte(lcd, LCD_INIT, sendAsCommand);
    vTaskDelay(pdMS_TO_TICKS(5));
    LCDsendByte(lcd, LCD_4BITS_MODE, sendAsCommand);
    vTaskDelay(pdMS_TO_TICKS(5));
    LCDsendByte(lcd, LCD_2LINES_35P, sendAsCommand);
    vTaskDelay(pdMS_TO_TICKS(5));
    LCDsendByte(lcd, LCD_EN_DIS_HID_CUR, sendAsCommand);
    vTaskDelay(pdMS_TO_TICKS(5));
    LCDsendByte(lcd, LCD_CLEAR, sendAsCommand);
    vTaskDelay(pdMS_TO_TICKS(5));
}

void LCDclear(LCD1602 *lcd){
    LCDsendByte(lcd, LCD_CLEAR, sendAsCommand);
}

void LCDsetCursor(LCD1602 *lcd, uint8_t pos_x, uint8_t pos_y){
    if (pos_x > 15) pos_x = 15;
    if (pos_y > 1) pos_y = 1;
    
    uint8_t position = 0x80 + (pos_y * 0x40) + pos_x;
    LCDsendByte(lcd, position, sendAsCommand);
}

void LCDprint(LCD1602 *lcd, const char *str){
    for(uint8_t i = 0; i < strlen(str); ++i){
        LCDsendByte(lcd, str[i], sendAsData);
    }
}

void LCDsetBackgroundLight(LCD1602 *lcd, BackgroundLightState state){
    lcd->BackgroundLight = state;
}