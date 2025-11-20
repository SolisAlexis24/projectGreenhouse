/**
 *************************************
 * @file: zeroCross.c
 * @author: Solis Hernandez Ian Alexis
 * @year: 2025
 * @licence: MIT
 * ***********************************
 */

#include "zeroCross.h"
#include "esp_err.h"
#include <stdint.h>

static const char *zx_TAG = "ZeroX";

#define ZERO_CROSS_PIN 13
#define DIMMER_PIN 33
#define ZERO_CROSS_LED 2

static gptimer_alarm_config_t internalAlarmConf = {
        .alarm_count = 8000,
        .flags.auto_reload_on_alarm = false,
};

static gptimer_handle_t zxTimer = NULL;

// @brief Interrupcion que activa un temporizador para habilitar el triac
static void IRAM_ATTR _risingEdgeISR(){
 	//esp_rom_delay_us(800);
	gpio_set_level(ZERO_CROSS_LED, 1);
	gptimer_set_raw_count(zxTimer, 0);
	gptimer_start(zxTimer); 
}

// @brief Activacion del TRIAC por 20 us
static bool _enableTRIAC(gptimer_handle_t timer, const gptimer_alarm_event_data_t *edata, void *user_ctx){
	gpio_set_level(DIMMER_PIN, 1);
	esp_rom_delay_us(20);
	gpio_set_level(DIMMER_PIN, 0);
	gpio_set_level(ZERO_CROSS_LED, 0);
	gptimer_stop(zxTimer);
	return false;
}


esp_err_t zeroCrossInit(){
	esp_err_t E;
	E = gpio_reset_pin(ZERO_CROSS_PIN);
	if(E){
		ESP_LOGE(zx_TAG, "Invalid GPIO pin for Zero cross");
		return E;
	}
	E = gpio_reset_pin(DIMMER_PIN);
	if(E){
		ESP_LOGE(zx_TAG, "Invalid GPIO pin for dimmer");
		return E;
	}
	// LED
	gpio_set_direction(ZERO_CROSS_LED, GPIO_MODE_OUTPUT);
	gpio_set_level(ZERO_CROSS_LED, 0);
	// Dimmer pin configuration
	gpio_set_direction(DIMMER_PIN, GPIO_MODE_OUTPUT);
	gpio_set_level(DIMMER_PIN, 0);
	// Zero Cross pin configuration
	gpio_set_direction(ZERO_CROSS_PIN, GPIO_MODE_INPUT);
	gpio_install_isr_service(0);
	gpio_set_intr_type(ZERO_CROSS_PIN, GPIO_INTR_POSEDGE);  
	gpio_isr_handler_add(ZERO_CROSS_PIN, _risingEdgeISR, NULL); 
	// timer configuration for dimmer 
	gptimer_config_t timer_config = {
		.clk_src = GPTIMER_CLK_SRC_DEFAULT,
		.direction = GPTIMER_COUNT_UP,
		.resolution_hz = 1 * 1000 * 1000,
	};
	E = gptimer_new_timer(&timer_config, &zxTimer);
	if(E){
		ESP_LOGE(zx_TAG, "Can't allocate timer configuration");
		return E;	
	}
	E = gptimer_set_alarm_action(zxTimer, &internalAlarmConf);
	if(E){
		ESP_LOGE(zx_TAG, "Can't set alarm action");
		return E;	
	}
	gptimer_event_callbacks_t zxCallback = {
		.on_alarm = _enableTRIAC,
	};
	E = gptimer_register_event_callbacks(zxTimer, &zxCallback, NULL);
	if(E){
		ESP_LOGE(zx_TAG, "Can't set callback function for TRIAC");
		return E;	
	}
	E = gptimer_enable(zxTimer);
	if(E){
		ESP_LOGE(zx_TAG, "Can't start timer");
		return E;	
	}
	return ESP_OK;
}

// @brief Change the timer callback time (activation time for TRIAC)
// @param activationTime New activation time
static void _setActivationTimeMS(uint64_t activationTime){
	internalAlarmConf.alarm_count = activationTime;
	gptimer_set_alarm_action(zxTimer, &internalAlarmConf);
}


esp_err_t setBulbPowerPerc(float powerPerc){
	if(NULL == zxTimer)
		return ESP_ERR_INVALID_ARG;

    if (powerPerc >= 1.0f)
        _setActivationTimeMS(0);
    else if (powerPerc >= 0.95f)
        _setActivationTimeMS(2060);
    else if (powerPerc >= 0.90f)
        _setActivationTimeMS(2696);
    else if (powerPerc >= 0.85f)
        _setActivationTimeMS(3157);
    else if (powerPerc >= 0.80f)
        _setActivationTimeMS(3538);
    else if (powerPerc >= 0.75f)
        _setActivationTimeMS(3874);
    else if (powerPerc >= 0.70f)
        _setActivationTimeMS(4179);
    else if (powerPerc >= 0.65f)
        _setActivationTimeMS(4464);
    else if (powerPerc >= 0.60f)
        _setActivationTimeMS(4734);
    else if (powerPerc >= 0.55f)
        _setActivationTimeMS(4993);
    else if (powerPerc >= 0.50f)
        _setActivationTimeMS(5245);
    else if (powerPerc >= 0.45f)
        _setActivationTimeMS(5493);
    else if (powerPerc >= 0.40f)
        _setActivationTimeMS(5738);
    else if (powerPerc >= 0.35f)
        _setActivationTimeMS(5984);
    else if (powerPerc >= 0.30f)
        _setActivationTimeMS(6232);
    else if (powerPerc >= 0.25f)
        _setActivationTimeMS(6487);
    else if (powerPerc >= 0.20f)
        _setActivationTimeMS(6750);
    else if (powerPerc >= 0.15f)
        _setActivationTimeMS(7030);
    else if (powerPerc >= 0.10f)
        _setActivationTimeMS(7334);
    else if (powerPerc >= 0.05f)
        _setActivationTimeMS(7688);
    else
        _setActivationTimeMS(8203);

    return ESP_OK;
}


