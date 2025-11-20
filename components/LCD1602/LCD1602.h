/**
 *************************************
 * @file: LCD1602.h
 * @author: Solis Hernandez Ian Alexis
 * @year: 2025
 * @licence: MIT
 * ***********************************
 */

#pragma once

#include <string.h>
#include "driver/i2c_master.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_rom_sys.h"
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

/**
 * @brief      Initialize the LCD at the given I2C address
 *
 * @param      lcd         LCD handler (To set configuration and bus master)
 * @param[in]  i2c_addr    I2C address of LCD
 * @param[in]  i2c_speed   I2C communication speed
 * @param      bus_handle  Master handler
 *
 * @return     
 * - ESP_ERR_INVALID_ARG If the master handler was not valid
 * - ESP_ERR_NO_MEM If there is not any memory
 * - ESP_ERR_TIMEOUT If communication could not be stablished
 * - ESP_OK If LCD configuration was successfull
 */
esp_err_t LCDinit(LCD1602 *lcd, uint8_t i2c_addr,int i2c_speed, i2c_master_bus_handle_t *bus_handle);
/**
 * @brief      Prints given test into LCD
 *
 * @param      lcd        LCD handler
 * @param[in]  str        String to be printed
 */
void LCDprint(LCD1602 *lcd, const char *str, ...);
/**
 * @brief      Clear the LCD content
 *
 * @param      LCD handler
 */
void LCDclear(LCD1602 *lcd);
/**
 * @brief      Set cursor into given position
 *
 * @param      LCD handler
 * @param[in]  pos_x  X position (0-15)
 * @param[in]  pos_y  Y position (0-1)
 */
void LCDsetCursor(LCD1602 *lcd, uint8_t pos_x, uint8_t pos_y);
/**
 * @brief      Prints *C into LCD
 *
 * @param      LCD handler
 */
void LCDprintCelsiusSymbol(LCD1602 *lcd);
/**
 * @brief      Prints % into LCD
 *
 * @param      LCD handler
 */
void LCDprintPercentageSymbol(LCD1602 *lcd);
/**
 * @brief      Sets intern variable that handles background light of LCD to given state
 *
 * @param      LCD handler
 * @param[in]  Background Light State (ON or OFF)
 */
void LCDsetBackgroundLight(LCD1602 *lcd, BackgroundLightState state);

/**
 * @brief      Send a byte to LCD thorug I2C
 *
 * @param      LCD handler
 * @param[in]  Byte to send
 * @param[in]  Send mode (command or data)
 *
 * @return     
 * - ESP_ERR_TIMEOUT If communication with LCD was not successfull
 * - ESP_OK If communication was successfull
 */
esp_err_t LCDsendByte(LCD1602 *lcd, uint8_t Byte, LCDsendMode mode);