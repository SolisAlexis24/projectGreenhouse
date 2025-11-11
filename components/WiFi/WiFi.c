/**
 *************************************
 * @file: WiFi.c
 * @author: Solis Hernandez Ian Alexis
 * @year: 2025
 * @licence: MIT
 * ***********************************
 */
#include "WiFi.h"


static EventGroupHandle_t wifiEventGroup;
static int _retryNum = 0;

static void _initPeripherialsAndDrivers(){

    ESP_ERROR_CHECK(esp_netif_init());

    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_create_default_wifi_sta();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

}

/**
 * @brief      WiFi handler: Tries to connect to an AP _retryNum - 1 times
 *
 * @param      arg         Arguments (not used)
 * @param[in]  event_base  Event base 
 * @param[in]  event_id    Event identifier
 * @param      event_data  Event data (not used)
 */
static void _WiFiHandler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data){
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
    	ESP_LOGI(WiFi_TAG, "Connecting to AP ...");
        esp_wifi_connect();
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        if (_retryNum < MAXIMUM_RETRY) {
            esp_wifi_connect();
            _retryNum++;
            ESP_LOGI(WiFi_TAG, "Reconnecting to AP ...");
        } else {
            xEventGroupSetBits(wifiEventGroup, WIFI_FAILURE);
        }
        ESP_LOGI(WiFi_TAG,"Failed to reconnect to AP :(");
	}
}

/**
 * @brief      IP handler: Print acquired IP
 *
 * @param      arg         Arguments (not used)
 * @param[in]  event_base  Event base 
 * @param[in]  event_id    Event identifier
 * @param      event_data  Event data (not used)
 */
static void _IPHandler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data){
	if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
	        ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data;
	        ESP_LOGI(WiFi_TAG, "Acquired static IP:" IPSTR, IP2STR(&event->ip_info.ip));
	        _retryNum = 0;
	        xEventGroupSetBits(wifiEventGroup, WIFI_SUCCESS);
	}
}

static void _configureConnection(){
    wifi_config_t wifi_config = {
        .sta = {
            .ssid = SSID,
            .password = PSSWD,
            .threshold.authmode = WIFI_AUTH_WPA2_PSK,
             .pmf_cfg = {
        	.capable = true,
        	.required = false
       		},
        },
    };
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA) );
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
}

esp_err_t WiFiInit(){
	esp_err_t errorStatus = WIFI_FAILURE;
	_initPeripherialsAndDrivers();

	wifiEventGroup = xEventGroupCreate();

	esp_event_handler_instance_t instance_any_id;
    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT,
                                                        ESP_EVENT_ANY_ID,
                                                        &_WiFiHandler,
                                                        NULL,
                                                        &instance_any_id));
	esp_event_handler_instance_t instance_got_ip;
    ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT,
                                                    IP_EVENT_STA_GOT_IP,
                                                    &_IPHandler,
                                                    NULL,
                                                    &instance_got_ip));

	_configureConnection();

    ESP_ERROR_CHECK(esp_wifi_start());

    ESP_LOGI(WiFi_TAG, "STA initialization finished");

    /* Waiting until either the connection is established 
     * or connection failed for the maximum number of re-tries*/
    EventBits_t bits = xEventGroupWaitBits(wifiEventGroup,
            WIFI_SUCCESS | WIFI_FAILURE,
            pdFALSE,
            pdFALSE,
            portMAX_DELAY);

    if (bits & WIFI_SUCCESS) {
        ESP_LOGI(WiFi_TAG, "Connected to AP");
        errorStatus = WIFI_SUCCESS;
    } else if (bits & WIFI_FAILURE) {
        ESP_LOGI(WiFi_TAG, "Failed to connect to AP");
        errorStatus = WIFI_FAILURE;
    } else {
        ESP_LOGE(WiFi_TAG, "Something weir happend");
        errorStatus = WIFI_FAILURE;
    }

    ESP_ERROR_CHECK(esp_event_handler_instance_unregister(IP_EVENT, IP_EVENT_STA_GOT_IP, instance_got_ip));
	ESP_ERROR_CHECK(esp_event_handler_instance_unregister(WIFI_EVENT, ESP_EVENT_ANY_ID, instance_any_id));

	vEventGroupDelete(wifiEventGroup);

	return errorStatus;
}


esp_err_t connectTCPServer(int mySocket){
	struct sockaddr_in serverInfo = {0};

	serverInfo.sin_family = AF_INET;
	inet_pton(AF_INET, SERVER_IP, &serverInfo.sin_addr);
	serverInfo.sin_port = SERVER_PORT;

	if(connect(mySocket, (struct sockaddr *)&serverInfo, sizeof(serverInfo)) != 0){
		ESP_LOGE(WiFi_TAG, "Failed to connect to %s", inet_ntoa(serverInfo.sin_addr.s_addr));
		return TCP_FAILURE;		
	}

	ESP_LOGI(WiFi_TAG, "Connected to TCP server");

	return TCP_SUCCESS;
}


esp_err_t sendSensorsDataToServer(int mySocket, float LM135Temp, float AM2302Hum, float AM2302Temp){
	int transactionStatus = TCP_SUCCESS;
    cJSON *root = cJSON_CreateObject();
	cJSON *sensors = cJSON_CreateArray();

    cJSON *lm135 = cJSON_CreateObject(); 
	cJSON_AddStringToObject(lm135, "sensor", "LM135");
	cJSON_AddNumberToObject(lm135, "temperature", LM135Temp);

    cJSON *am2302 = cJSON_CreateObject(); 
    cJSON_AddStringToObject(am2302, "sensor", "AM2302");
    cJSON_AddNumberToObject(am2302, "temperature", AM2302Temp);
    cJSON_AddNumberToObject(am2302, "humidity", AM2302Hum);

    cJSON_AddItemToArray(sensors, lm135);
    cJSON_AddItemToArray(sensors, am2302);

    cJSON_AddItemToObject(root, "sensors", sensors);

	char *json_str = cJSON_Print(root);

    int err = send(mySocket, json_str, strlen(json_str), 0);
    if (err < 0) {
        ESP_LOGE(WiFi_TAG, "Error occurred during sending: errno %d", errno);
        transactionStatus = TCP_FAILURE;
    }
    cJSON_free(json_str);
   	cJSON_Delete(root);
    return transactionStatus;
}