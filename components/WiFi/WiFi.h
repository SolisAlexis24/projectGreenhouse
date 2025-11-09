/**
 *************************************
 * @file: LCD1602.h
 * @author: Solis Hernandez Ian Alexis
 * @purpose: Header file for WiFi configuration and usage
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




#define MAXIMUM_RETRY 8
#define WIFI_SUCCESS 1 << 0
#define WIFI_FAILURE 1 << 1
#define TCP_SUCCESS 1 << 0
#define TCP_FAILURE 1 << 1

#define SSID "INFINITUMC023_2.4"
#define PSSWD "Uranioxd235"
#define SERVER_IP "192.168.1.172"
#define SERVER_PORT htons(42069)

static const char *WiFi_TAG = "WiFi";


/**
 * @brief      Initializes the WiFi interface and connects to an AP
 *
 * @return     
 * - WIFI_SUCCESS	If WiFi was successfully initialized and connection with AP was successfull
 * - WIFI_FAILURE 	If connection with AP was not successfull 
 */
esp_err_t WiFiInit();

/**
 * @brief      Connects to a tcp server using specified socket.
 *
 * @param[in]  Socket to connect with
 *
 * @return
 * - TCP_SUCCESS If connection with server was successfull
 * - TCP_FAILURE If connection with server was not successfull
 */
esp_err_t connectTCPServer(int mySocket);


esp_err_t sendSensorsDataToServer(int mySocket, float LM135Temp, float AM2302Hum, float AM2302Temp);
