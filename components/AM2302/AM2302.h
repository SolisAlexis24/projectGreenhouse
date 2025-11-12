/**
 *************************************
 * @file: AM2302.h
 * @author: Solis Hernandez Ian Alexis
 * @year: 2025
 * @licence: MIT
 * ***********************************
 */

#pragma once

#include <stdint.h>
#include <inttypes.h>
#include "driver/gpio.h"
#include "hal/gpio_types.h"
#include "soc/gpio_num.h"
#include "esp_err.h"
#include "esp_log.h"
#include "esp_log_level.h"
#include "freertos/FreeRTOS.h"
#include "esp_rom_sys.h"


#define NO_DURATION_RECORD NULL
#define LOW_LEVEL 0
#define HIGH_LEVEL 1
#define TIMEOUT_LOW_LEVEL 65
#define TIMEOUT_HIGH_LEVEL 75
#define AM2302_DATA_BITS 40
#define MINIMUM_RESOLUTION_DELAY 2

typedef struct{
	gpio_num_t pin;
	bool readyToUse;
	float temperature;
	float humidity;
	bool checksumOK;
	portMUX_TYPE Spinlock;
}AM2302Handler;

/**
 * @brief      Ininialize AM2302 pin and tries to read information
 *
 * @param      sh    Sensor handler that will be used with the rest of functions
 * @param[in]  pin   The pin to be used
 *
 * @return
 * - ESP_OK If given pin got initialized correctly
 * - ESP_ERR_INVALID_ARG If given pin was not appropiate for initialization or if sh is NULL
 * - ESP_ERR_TIMEOUT If AM2302 does not respond when trying to read
 * - ESP_ERR_INVALID_RESPONSE If checksum of response does not match
 */
esp_err_t AM2302init(AM2302Handler *sh, gpio_num_t pin);

/**
 * @brief      Send start signals and call function to fetch sensor data
 *
 * @param      sh[in/out]    Sensor Handler
 *
 * @return    
 * - ESP_OK: 					If data was successfully collected and checksum matchs
 * - ESP_ERR_TIMEOUT: 			If the program kept waiting more than usual
 * - ESP_ERR_INVALID_RESPONSE:	If checksum does not match 
 * 
 * @note If functions returns with OK state, humidity and temperature data will be located in respective handler fields.
 * 
 */
esp_err_t AM2302read(AM2302Handler* sh);