/**
 *************************************
 * @file: PWM.c
 * @author: Solis Hernandez Ian Alexis
 * @year: 2025
 * @licence: MIT
 * ***********************************
 */

#include "PWM.h"
#include "driver/ledc.h"
#include "esp_err.h"
#include "esp_log.h"
#include "hal/ledc_types.h"
#include "soc/clk_tree_defs.h"
#include <stdbool.h>
#include <stdint.h>

esp_err_t FanInit(FanHandler *fanHan, gpio_num_t pin, ledc_channel_t channel){
	if(!fanHan)
		return ESP_ERR_INVALID_ARG;

	esp_err_t errorStatus;
	ledc_timer_config_t timer_conf = {
		.speed_mode = DEF_MODE,
		.duty_resolution = LEDC_TIMER_11_BIT,
		.timer_num = LEDC_TIMER_0,
		.freq_hz = 25000,
		.clk_cfg = LEDC_AUTO_CLK
	};
	errorStatus = ledc_timer_config(&timer_conf);
	if(errorStatus){
		ESP_LOGE(PWM_TAG, "Error during configuring timer");
		return errorStatus;
	}

	ledc_channel_config_t channel_conf = {
		.gpio_num = pin,
		.speed_mode = DEF_MODE,
		.channel = channel,
		.timer_sel = LEDC_TIMER_0,
		.intr_type = LEDC_INTR_DISABLE,
		.duty = 0,
		.hpoint = 0
	};
	errorStatus = ledc_channel_config(&channel_conf);
	if(errorStatus){
		ESP_LOGE(PWM_TAG, "Error during configuring channel");
		return errorStatus;
	}
	errorStatus = ledc_fade_func_install(0);
	if(errorStatus){
		ESP_LOGE(PWM_TAG, "Error during fading initialization");
		return errorStatus;
	}
	fanHan->PWMchannel = channel;
	fanHan->PWMmode = DEF_MODE;
	return ESP_OK;
}

esp_err_t setFanDutyCyclePerc(FanHandler *fanHan, float DCpercentage){
	if(!fanHan)
		return ESP_ERR_INVALID_ARG;

	uint32_t dutyCycleInt;
	if(DCpercentage >= 1.0 || DCpercentage <= 0.0)
		dutyCycleInt = MAX_NUM_11_BITS;
	else
		dutyCycleInt = (uint32_t)(MAX_NUM_11_BITS * DCpercentage);

	return ledc_set_duty_and_update(fanHan->PWMmode, fanHan->PWMchannel, dutyCycleInt, 0);
}

float getFanDutyCyclePerc(FanHandler *fanHan){
	if(!fanHan)
		return -1.0;

	uint32_t dutyOutput = ledc_get_duty(fanHan->PWMmode, fanHan->PWMchannel);
	if(LEDC_ERR_DUTY == dutyOutput)
		return -1.0;

	return (float)dutyOutput / (float)MAX_NUM_11_BITS;
}