#include "LCD1602.h"

void LCDpulseEnable(LCD1602 *lcd, uint8_t data)
{
    uint8_t buffer;
    
    // Set enable bit high
    buffer = data | LCD_EN_FLAG;
    if (lcd->BackgroundLight == BackgroundLightON) {
        buffer |= LCD_BL_FLAG;
    }
    ESP_ERROR_CHECK(i2c_master_transmit(lcd->i2c_handler, &buffer, 1, I2C_MASTER_TIMEOUT_MS / portTICK_PERIOD_MS));
    vTaskDelay(1 / portTICK_PERIOD_MS);
    
    // Set enable bit low
    buffer = data & ~LCD_EN_FLAG;
    if (lcd->BackgroundLight == BackgroundLightON) {
        buffer |= LCD_BL_FLAG;
    }
    ESP_ERROR_CHECK(i2c_master_transmit(lcd->i2c_handler, &buffer, 1, I2C_MASTER_TIMEOUT_MS / portTICK_PERIOD_MS));
    vTaskDelay(1 / portTICK_PERIOD_MS);
}

void LCDsendByte(LCD1602 *lcd, uint8_t Byte, LCDsendMode mode)
{
    if (!lcd->isConnected) 
        return;

    uint8_t high_nibble, low_nibble;
    uint8_t flags = 0x00; // RW = 0 (write)
    
    if (mode == sendAsData) {
        flags |= LCD_RS_FLAG; // RS = 1 para datos
    }
    // RS = 0 para comandos
    
    if (lcd->BackgroundLight == BackgroundLightON) {
        flags |= LCD_BL_FLAG;
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
    
    // ESPERAR MÁS TIEMPO PARA ESTABILIZACIÓN DE VOLTAJE
    vTaskDelay(100 / portTICK_PERIOD_MS);  // Aumentado de 50ms a 100ms
    
    
    // SECUENCIA DE INICIALIZACIÓN MÁS ROBUSTA
    // Intentar forzar el modo 8-bit tres veces
    for(int i = 0; i < 3; i++) {
        LCDsendByte(lcd, 0x30, sendAsCommand);
        vTaskDelay(10 / portTICK_PERIOD_MS);
    }
    
    // Cambiar a modo 4-bit
     LCDsendByte(lcd, 0x20, sendAsCommand);
    vTaskDelay(1 / portTICK_PERIOD_MS);
    
    // Configurar función: 2 líneas, fuente 5x8
    LCDsendByte(lcd, 0x28, sendAsCommand);
    vTaskDelay(1 / portTICK_PERIOD_MS);
    
    // Apagar display
    LCDsendByte(lcd, 0x08, sendAsCommand);
    vTaskDelay(1 / portTICK_PERIOD_MS);
    
    // Limpiar display
    LCDsendByte(lcd, 0x01, sendAsCommand);
    vTaskDelay(20 / portTICK_PERIOD_MS);
    
    // Modo de entrada: incrementar cursor, no shift
    LCDsendByte(lcd, 0x06, sendAsCommand);
    vTaskDelay(1 / portTICK_PERIOD_MS);
    
    // Encender display, cursor off, blink off
    LCDsendByte(lcd, 0x0C, sendAsCommand);
    vTaskDelay(1 / portTICK_PERIOD_MS);
    
    vTaskDelay(100 / portTICK_PERIOD_MS);
}

void LCDclear(LCD1602 *lcd){
    LCDsendByte(lcd, 0x01, sendAsCommand);
    vTaskDelay(2 / portTICK_PERIOD_MS);
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
        vTaskDelay(1 / portTICK_PERIOD_MS);
    }
}

void LCDsetBackgroundLight(LCD1602 *lcd, BackgroundLightState state){
    lcd->BackgroundLight = state;
}