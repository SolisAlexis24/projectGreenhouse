/**
 *************************************
 * @file: ADC.c
 * @author: Solis Hernandez Ian Alexis
 * @year: 2025
 * @licence: MIT
 * ***********************************
 */

#include "ADC.h"


esp_err_t ADCconfigUnitBasic(ADCHandler *adcHan, adc_unit_t unit){
	if(!adcHan)
		return ESP_ERR_INVALID_ARG;

	adc_oneshot_unit_init_cfg_t initConfig = {
    .unit_id = unit,
    .ulp_mode = ADC_ULP_MODE_DISABLE
	};

	esp_err_t errorStatus = adc_oneshot_new_unit(&initConfig, &adcHan->unitHandler);
	if(errorStatus){
		adcHan->unitHandler = NULL;
		ESP_LOGE(TAG_ADC, "Unit configuration failed");
	}

	adcHan->unitID = unit;
	return errorStatus;
}

esp_err_t ADCConfigChannel(ADCHandler *adcHan, adc_atten_t attenuation, adc_bitwidth_t outputResolution, adc_channel_t channel){
	if(!adcHan || !adcHan->unitHandler)
		return ESP_ERR_INVALID_ARG;

	adc_oneshot_chan_cfg_t config = {
		.bitwidth = outputResolution,
		.atten = attenuation
	};

	esp_err_t error_status = adc_oneshot_config_channel(adcHan->unitHandler, channel, &config);
	if(!error_status){
		adcHan->outputResolution = outputResolution;
		adcHan->channel = channel;
		adcHan->attenuation = attenuation;
	}
	else{
		 ESP_LOGE(TAG_ADC, "Channel configuration failed");
	}

    adc_cali_line_fitting_config_t cali_config = {
        .unit_id = adcHan->unitID,
        .atten = adcHan->attenuation,
        .bitwidth = adcHan->outputResolution,
    };
    error_status = adc_cali_create_scheme_line_fitting(&cali_config, &adcHan->calibrationHandler);

    if(error_status){
    	ESP_LOGE(TAG_ADC, "ADC calibration failed");
    }

	return error_status;
}

esp_err_t ADCread(ADCHandler *adcHan){
	if(!adcHan || !adcHan->unitHandler)
		return ESP_ERR_INVALID_ARG;

	return adc_oneshot_read(adcHan->unitHandler, adcHan->channel, &adcHan->rawData);
}


esp_err_t LM35init(LM35Handler *lm35_han, ADCHandler *adcHan){
	if(!adcHan)
		return ESP_ERR_INVALID_ARG;
	lm35_han->adc = adcHan;
	return ESP_OK;
}

esp_err_t LM35read(LM35Handler *lm35Han){
	esp_err_t errorStatus = ADCread(lm35Han->adc);
	if(errorStatus){
		lm35Han->temperature = 0;
		ESP_LOGE(TAG_ADC, "LM35 acquisition failed");
		return errorStatus;
	}
	int voltage = 0;
	adc_cali_raw_to_voltage(lm35Han->adc->calibrationHandler, lm35Han->adc->rawData, &voltage);
	lm35Han->temperature = (float)voltage / 10.0;
	return ESP_OK;
}