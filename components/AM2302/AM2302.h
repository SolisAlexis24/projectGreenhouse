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


static const char *AM2302_TAG = "AM2302";
typedef struct{
	gpio_num_t pin;
	bool readyToUse;
	float temperature;
	float humidity;
	bool checksumOK;
	portMUX_TYPE Spinlock;
}AM2302Handler;

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
esp_err_t _AM2302fetchData(AM2302Handler* sh);

/**
 * @brief      Send start signal to AM2302 sensor: 2 ms low
 *
 * @param[in]      sh Sensor Handler
 * 
 * @warning    Internal function, do not use
 */
void _AM2302sendStartSignal(AM2302Handler* sh);

/**
 * @brief      Relase the bus so AM2302 can change pin level to transmit ACK and then data
 *
 * @param[in]      sh    Sensor Handler
 */
void _AM2302relaseBus(AM2302Handler* sh);


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
esp_err_t _AM2302AwaitPinLevel_us(AM2302Handler* sh, uint8_t *duration, int expected_level, uint8_t timeout);