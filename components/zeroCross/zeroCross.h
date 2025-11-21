/**
 *************************************
 * @file: zeroCross.h
 * @author: Solis Hernandez Ian Alexis
 * @year: 2025
 * @licence: MIT
 * ***********************************
 */

#pragma once
#include "esp_err.h"
#include "soc/gpio_num.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "esp_attr.h"
#include "hal/gpio_types.h"
#include "driver/gptimer.h"


#define ZERO_CROSS_PIN 13
#define DIMMER_PIN 33
#define ZERO_CROSS_LED 2
#define MAX_BULB_POWER 1.0
#define MIN_BUBL_POWER 0.0

/**
 * @brief      Init zero cross detection (pins and timer)
 *
 * @return
 * - ESP_OK Initialization was successfull
 * - ESP_ERR_INVALID_ARG Parameter error
 * @warning This module uses GPIO 2 LED for indicating zero cross detection
 */
esp_err_t zeroCrossInit();

/**
 * @brief      Modify the power of the bulb
 *
 * @param[in]  powerPerc  Power requested range [0-1]
 *
 * @return
 * - ESP_ERR_INVALID_ARG if Timer was not initialized (zeroCrossInit)
 * - ESP_OK Sucess
 */
esp_err_t setBulbPowerPerc(float powerPerc);
