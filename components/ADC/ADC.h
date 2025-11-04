/**
 *************************************
 * @file: ADC.h
 * @author: Solis Hernandez Ian Alexis
 * @year: 2025
 * @licence: MIT
 * ***********************************
 */

#pragma once

#include "hal/adc_types.h"
#include "esp_adc/adc_oneshot.h"
#include "esp_err.h"
#include "esp_adc/adc_cali.h"
#include "esp_adc/adc_oneshot.h"
#include "esp_log.h"
#include "hal/adc_types.h"




typedef struct {
	adc_oneshot_unit_handle_t unitHandler;
	adc_unit_t unitID;
	adc_channel_t channel;
	adc_bitwidth_t outputResolution;
	adc_atten_t attenuation;
	adc_cali_handle_t calibrationHandler;
	int rawData;
}ADCHandler;


typedef struct{
	ADCHandler *adc;
	float temperature;
}LM35Handler;


static const char *TAG_ADC = "ADC";


/**
 * @brief      Configure an ADC unit using the default clock source and no ULP
 *
 * @param  		adcHan      ADC handler that will contain configured unit handler
 * @param[in]	unit		Unit to be configued
 *
 * @return     
 * - ESP_OK: 				On success 
 * - ESP_ERR_INVALID_ARG:	Ivalid arguments
 * - ESP_ERR_NO_MEM:		No memory
 * - ESP_ERR_NOT_FOUND:		The ADC peripheral to be claimed is already in use
 * - ESP_FAIL:				Clock source isn't initialised correctly
 * 
 * @note If unit configuration fails, ADCunit will be a null pointer
 */
esp_err_t ADCconfigUnitBasic(ADCHandler *adcHan, adc_unit_t unit);

/**
 * @brief      Configure a channel from the unit handler contained in ADCHandler
 *
 * @param      adcHan            ADC handler that will contain specified channel
 * @param[in]  attenuation       The attenuation for the channel
 * @param[in]  outputResolution  The output resolution for measurements
 * @param[in]  channel           The channel where to read from
 *
 * @return
 * - ESP_OK: 				On success 
 * - ESP_ERR_INVALID_ARG:	Ivalid arguments
 * 
 * @note If channel configuration fails, ADCchannel will not be initialized
 */
esp_err_t ADCConfigChannel(ADCHandler *adcHan, adc_atten_t attenuation, adc_bitwidth_t outputResolution, adc_channel_t channel);

/**
 * @brief      Read and store ADC measurement
 *
 * @param      adcHan  ADCH handler where the raw information will be stored
 *
 * @return
 * - ESP_OK:                On success
 * - ESP_ERR_INVALID_ARG:   Invalid arguments
 * - ESP_ERR_TIMEOUT:       Timeout, the ADC result is invalid
 */
esp_err_t ADCread(ADCHandler *adcHan);


/**
 * @brief      Bind a LM35 handler and a ADC Handler
 *
 * @param      lm35_han  LM35 handler
 * @param      adcHan    ADC Handler
 *
 * - ESP_OK: 				On success 
 * - ESP_ERR_INVALID_ARG:	Handler is not initialized
 */
esp_err_t LM35init(LM35Handler *lm35_han, ADCHandler *adcHan);


/**
 * @brief      Read ADC port bindend to handler and store measurement in its temperature field
 *
 * @param      lm35Han  LM35 handler
 *
 * @return     
 * - ESP_ERR_INVALID_ARG:	Handler ir not initialized
 * - ESP_OK:				On success
 */
esp_err_t LM35read(LM35Handler *lm35Han);