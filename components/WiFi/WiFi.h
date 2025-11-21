/**
 *************************************
 * @file: WiFi.h
 * @author: Solis Hernandez Ian Alexis
 * @year: 2025
 * @licence: MIT
 * ***********************************
 */

#pragma once
#include <netdb.h>  
#include "cJSON.h"
#include "esp_wifi.h"
#include "esp_err.h"
#include "esp_log.h"
#include "cJSON.h"
#include "cc.h"
#include "esp_err.h"
#include "esp_event.h"
#include "esp_event_base.h"
#include "esp_log.h"
#include "esp_netif_types.h"
#include "esp_wifi.h"
#include "esp_wifi_types_generic.h"
#include "freertos/idf_additions.h"
#include "lwip/inet.h"
#include "lwip/sockets.h"
#include <stdio.h>
#include <strings.h>
#include <unistd.h>


#define MAXIMUM_RETRY 8
#define WIFI_SUCCESS 1 << 0
#define WIFI_FAILURE 1 << 1
#define TCP_SUCCESS 1 << 0
#define TCP_FAILURE 1 << 1


/**
 * @brief      			Initializes the WiFi interface and connects to an AP
 * @param[in] 	ssid	Access point name
 * @param[in]	psswd 	Password for access point
 *
 * @return     
 * - WIFI_SUCCESS	If WiFi was successfully initialized and connection with AP was successfull
 * - WIFI_FAILURE 	If connection with AP was not successfull 
 */
esp_err_t WiFiInit(const char ssid[], const char psswd[]);

/**
 * @brief      Connects to a tcp server using specified socket.
 *
 * @param[in]  mySocket 	Socket to connect with
 * @param[in]  ip 	  		Server IP
 * @param[in]  port 		Server port
 *
 * @return
 * - TCP_SUCCESS If connection with server was successfull
 * - TCP_FAILURE If connection with server was not successfull
 */
esp_err_t connectTCPServer(int mySocket, const char ip[], in_port_t port);

/**
 * @brief      Sends given sensors data to server as a JSON
 *
 * @param[in]  mySocket    Socket to use
 * @param[in]  LM135Temp   LM135 temperature
 * @param[in]  AM2302Hum   AM2302 humidity
 * @param[in]  AM2302Temp  AM2302 temperature
 *
 * @return
 * - TCP_SUCCESS If data was delivered successfully
 * - TCP_FAILURE If data failed to be sent
 */
esp_err_t sendSensorsDataToServer(int mySocket, float LM135Temp, float AM2302Hum, float AM2302Temp);

/**
 * @brief      When an error has ocurred after trying to parse a JSON, prints such error
 */
void printJSONParsingError();

/**
 * @brief      Decodes JSON message from server, extracting function name and argument
 *
 * @param[in]  	buffer    Socket input buffer (JSON received)
 * @param[out]  funcName  Function to execute name
 * @param[out]  arg       Argument for function
 */
void decodeJSONServerMessage(const char buffer[], char funcName[], float *arg);