#pragma once

#include <string.h>
#include "driver/i2c_master.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_err.h"
#include <stdint.h>

#define I2C_MASTER_TIMEOUT_MS       1000

#define LCD_I2C_ADDR                0x27
#define LCD_EN_BIT                  0x04  // Enable bit
#define LCD_RW_BIT                  0x02  // Read/Write bit (0 = write)
#define LCD_RS_BIT                  0x01  // Register Select bit
#define LCD_BL_BIT                  0x08  // Backlight bit
#define LCD_INIT                    0x33
#define LCD_4BITS_MODE              0x32
#define LCD_2LINES_35P              0x28
#define LCD_EN_DIS_HID_CUR          0x0C
#define LCD_CLEAR                   0x01

#define DEGREE_SYMBOL   0xDF
#define PERCENTAGE_SYMBOL 0x25


typedef enum{
    sendAsCommand = 0,
    sendAsData
} LCDsendMode;

typedef enum{
    BackgroundLightOFF = 0,
    BackgroundLightON
} BackgroundLightState;

typedef struct{
    i2c_device_config_t i2c_config;
    i2c_master_dev_handle_t i2c_handler;
    BackgroundLightState BackgroundLight;
    bool isConnected;
} LCD1602;

esp_err_t LCDinit(LCD1602 *lcd, uint8_t i2c_addr,int i2c_speed, i2c_master_bus_handle_t *bus_handle);
esp_err_t LCDsendByte(LCD1602 *lcd, uint8_t Byte, LCDsendMode mode);
void LCDclear(LCD1602 *lcd);
void LCDsetCursor(LCD1602 *lcd, uint8_t pos_x, uint8_t pos_y);
void LCDprint(LCD1602 *lcd, const char *str, ...);
void LCDprintCelsiusSymbol(LCD1602 *lcd);
void LCDprintPercentageSymbol(LCD1602 *lcd);
void LCDsetBackgroundLight(LCD1602 *lcd, BackgroundLightState state);
esp_err_t LCDpulseEnable(LCD1602 *lcd, uint8_t data);