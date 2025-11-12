/**
 *************************************
 * @file: PWM.h
 * @author: Solis Hernandez Ian Alexis
 * @year: 2025
 * @licence: MIT
 * ***********************************
 */

#include "driver/ledc.h"
#include "esp_err.h"
#include "hal/ledc_types.h"
#include "soc/gpio_num.h"
#include "esp_log.h"
#include "hal/ledc_types.h"
#include "soc/clk_tree_defs.h"
#include <stdbool.h>
#include <stdint.h>

#define DEF_MODE LEDC_LOW_SPEED_MODE
#define MAX_NUM_11_BITS 2048

typedef struct{
	ledc_channel_t PWMchannel;
	ledc_mode_t PWMmode;
}FanHandler;

/**
 * @brief      Initialize given channel into specified pin and bind such information in the handler
 *
 * @param      fanHan   Fan handler which will be binded to the configuration
 * @param[in]  pin      GPIO pin for PWM
 * @param[in]  channel  PWM channel
 *
 * @return
 * - ESP_OK Success
 * - ESP_ERR_INVALID_ARG Parameter error
 * - ESP_FAIL Can not find a proper pre-divider number base on the given frequency and the current duty_resolution.
 * - ESP_ERR_INVALID_STATE Timer cannot be de-configured because timer is not configured or is not paused
 * - ESP_ERR_NOT_FOUND Failed to find available interrupt source
 */
esp_err_t FanInit(FanHandler *fanHan, gpio_num_t pin, ledc_channel_t channel);

/**
 * @brief      Sets the fan duty cycle percentage [0-1]
 *
 * @param      fanHan        Fan handler which have an initialized channel
 * @param[in]  DCpercentage  Duty cycle to be set
 *
 * @return
 * - ESP_OK Success
 * - ESP_ERR_INVALID_STATE Channel not initialized
 * - ESP_ERR_INVALID_ARG Parameter error
 * - ESP_FAIL Fade function init error
 */
esp_err_t setFanDutyCyclePerc(FanHandler *fanHan, float DCpercentage);

/**
 * @brief      Gets the fan duty cycle percentage
 *
 * @param      fanHan  Fan handler which have an initialized channel
 *
 * @return     Duty cycle if channel is initialized, -1.0 other way
 */
float getFanDutyCyclePerc(FanHandler *fanHan);