/**
 *************************************
 * @file: AM2302,c
 * @author: Solis Hernandez Ian Alexis
 * @year: 2025
 * @licence: MIT
 * ***********************************
 */

#include "AM2302.h"

esp_err_t AM2302init(AM2302Handler *sh, gpio_num_t pin){
	if(!sh)
		return ESP_ERR_INVALID_ARG;

	sh->pin = pin;
	esp_err_t statusR = gpio_reset_pin(pin);
	esp_err_t statusS = gpio_set_direction(pin, GPIO_MODE_INPUT_OUTPUT_OD);
	esp_err_t statusL = gpio_set_level(pin, 1);
	if (ESP_ERR_INVALID_ARG == statusR || ESP_ERR_INVALID_ARG == statusS || ESP_ERR_INVALID_ARG == statusL){
		ESP_LOGI(AM2302_TAG, "Pin not valid");
		return ESP_ERR_INVALID_ARG;
	}
	sh->readyToUse = true;
	spinlock_initialize(&sh->Spinlock);
	return AM2302read(sh);
}


/**
 * @brief      Send start signal to AM2302 sensor: 2 ms low
 *
 * @param[in]      sh Sensor Handler
 * 
 * @warning    Internal function, do not use
 */
static void _AM2302sendStartSignal(AM2302Handler* sh){
	gpio_set_level(sh->pin, LOW_LEVEL);
	esp_rom_delay_us(1000);
}


/**
 * @brief      Relase the bus so AM2302 can change pin level to transmit ACK and then data
 *
 * @param[in]      sh    Sensor Handler
 */
static void _AM2302relaseBus(AM2302Handler* sh){
	gpio_set_level(sh->pin, HIGH_LEVEL);
}


/**
 * @brief Waits for the AM2302 data line to reach a specific logic level within a timeout period.
 *
 * This function continuously samples the GPIO associated with the AM2302 sensor and waits
 * until it matches the expected logic level. It delays approximately 2 microseconds per 
 * iteration to avoid busy-waiting too tightly. If the expected level is detected before the timeout expires, 
 * the elapsed time is optionally stored in `duration`. Otherwise, the function returns a timeout error.
 *
 * @param[in]  sh              Pointer to an initialized AM2302Handler structure.
 * @param[out] duration        Optional pointer to store the time (in microseconds)
 *                             elapsed until the expected level was detected. Can be NULL.
 * @param[in]  expected_level  The logic level to wait for (0 for LOW, 1 for HIGH).
 * @param[in]  timeout         Maximum time to wait in microseconds before returning an error.
 *
 * @return
 *     - ESP_OK: The expected level was detected within the timeout period.  
 *     - ESP_ERR_TIMEOUT: The timeout expired before detecting the expected level.  
 *
 * @note This function does not measure how long the line remains at the expected level;
 *       it only measures how long it takes for the level to appear.
 */
static esp_err_t _AM2302AwaitPinLevel_us(AM2302Handler* sh, uint8_t *duration, int expected_level, uint8_t timeout){
	for(uint8_t i = 0; i < timeout; i += MINIMUM_RESOLUTION_DELAY){
		esp_rom_delay_us(MINIMUM_RESOLUTION_DELAY);
		if(expected_level == gpio_get_level(sh->pin)){
			if(duration)
				*duration = i;
			return ESP_OK;
		}
	}
	ESP_LOGE(AM2302_TAG, "Timeout has expired after %u [us] awaiting for the bus to be set to %d", timeout, expected_level);
	return ESP_ERR_TIMEOUT;
}


/**
 * @brief      Fetch 40 bits of infotmation after the start signals
 *
 * @param      sh[in/out]    Sensor Handler
 *
 * @return    
 * - ESP_OK: 					If data was successfully collected and checksum matchs
 * - ESP_ERR_TIMEOUT: 			If the program kept waiting more than usual
 * - ESP_ERR_INVALID_RESPONSE:	If checksum does not match 
 */
static esp_err_t _AM2302fetchData(AM2302Handler* sh){
	uint8_t highLevelDuration;
	uint8_t lowLevelDuration;
	uint8_t rawData[5] = {0};
	esp_err_t error_state;

	for(uint8_t i = 0; i < AM2302_DATA_BITS; ++i){		
		error_state = _AM2302AwaitPinLevel_us(sh, &lowLevelDuration, HIGH_LEVEL, TIMEOUT_LOW_LEVEL);
		if(error_state)
			return error_state;
		

		error_state = _AM2302AwaitPinLevel_us(sh, &highLevelDuration, LOW_LEVEL, TIMEOUT_HIGH_LEVEL);
		if(error_state)
			return error_state;

	    rawData[i / 8] <<= 1;
	    if (highLevelDuration > lowLevelDuration)
	        rawData[i / 8] |= 1;
	}

	uint8_t expectedChecksum = (rawData[0] + rawData[1] + rawData[2] + rawData[3]) & 0xFF;
	if(rawData[4] != expectedChecksum){
		vPortExitCritical(&sh->Spinlock);
		ESP_LOGE(AM2302_TAG, "Expected checksum: %02X", expectedChecksum);
		ESP_LOGE(AM2302_TAG, "Received checksum: %02X", rawData[4]);
		ESP_LOGE(AM2302_TAG, "Humifity high: %02X", rawData[0]);
		ESP_LOGE(AM2302_TAG, "Humidity low: %02X", rawData[1]);
		ESP_LOGE(AM2302_TAG, "Temperature high: %02X", rawData[2]);
		ESP_LOGE(AM2302_TAG, "Temperature low: %02X", rawData[3]);
		sh->humidity = 0.0;
		sh->temperature = 0.0;
		return ESP_ERR_INVALID_RESPONSE;
	}

	sh->humidity = ((rawData[0] << 8) | rawData[1])/10.f;
    sh->temperature = ((rawData[2] << 8) | rawData[3])/10.f;
    return ESP_OK;
}


esp_err_t AM2302read(AM2302Handler* sh){
	if(!sh)
		return ESP_ERR_INVALID_ARG;
	if(!sh->readyToUse)
		return ESP_ERR_INVALID_ARG;
	
	esp_err_t error_state;

	vPortEnterCritical(&sh->Spinlock);
	_AM2302sendStartSignal(sh);
	_AM2302relaseBus(sh);

	error_state = _AM2302AwaitPinLevel_us(sh, NO_DURATION_RECORD, LOW_LEVEL, 40);
	if(error_state){
		vPortExitCritical(&sh->Spinlock);
		return error_state;
	}
	
	error_state = _AM2302AwaitPinLevel_us(sh, NO_DURATION_RECORD, HIGH_LEVEL, 80);
	if(error_state){
		vPortExitCritical(&sh->Spinlock);
		return error_state;
	}
	
	error_state = _AM2302AwaitPinLevel_us(sh, NO_DURATION_RECORD, LOW_LEVEL, 80);
	if(error_state){
		vPortExitCritical(&sh->Spinlock);
		return error_state;
	}
	
	error_state = _AM2302fetchData(sh);
	vPortExitCritical(&sh->Spinlock);

	return error_state;
}