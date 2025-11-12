/**
 *************************************
 * @file: ADC.c
 * @author: Solis Hernandez Ian Alexis
 * @year: 2025
 * @licence: MIT
 * ***********************************
 */

#include "ADC.h"

static const char *TAG_ADC = "ADC";


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

esp_err_t ADCconfigChannel(ADCHandler *adcHan, adc_atten_t attenuation, adc_bitwidth_t outputResolution, adc_channel_t channel){
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


esp_err_t LM135init(LM135Handler *lm35_han, ADCHandler *adcHan){
	if(!adcHan)
		return ESP_ERR_INVALID_ARG;
	if(adcHan->attenuation != ADC_ATTEN_DB_12){
		ESP_LOGI(TAG_ADC, "Attenuation must be 12 dB");
		return ESP_ERR_INVALID_ARG;
	}
	lm35_han->adc = adcHan;
	ESP_LOGI(TAG_ADC, "LM35 initialized correctly");
	return ESP_OK;
}

esp_err_t LM135read(LM135Handler *lm135Han){
	esp_err_t errorStatus = ADCread(lm135Han->adc);
	if(errorStatus){
		lm135Han->temperature = 0;
		ESP_LOGE(TAG_ADC, "LM135 acquisition failed");
		return errorStatus;
	}
	int voltage = 0;
	adc_cali_raw_to_voltage(lm135Han->adc->calibrationHandler, lm135Han->adc->rawData, &voltage);
	lm135Han->temperature = ((float)voltage*0.1) - 273.25;
	return ESP_OK;
}