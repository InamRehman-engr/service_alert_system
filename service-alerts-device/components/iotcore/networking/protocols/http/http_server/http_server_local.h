

#ifndef _HTTP_SERVER_LOCAL_H
#define _HTTP_SERVER_LOCAL_H

#define WIFI_PROVISION_VIA_HTTP_SERVER

#include "esp_wifi.h"
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "freertos/queue.h"
#include "freertos/semphr.h"
#include "freertos/task.h"
#include <esp_http_server.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#if (ESP_IDF_VERSION_MAJOR > 4)
#include "esp_netif.h"
#else
#if (ESP_IDF_VERSION_MAJOR == 4) && (ESP_IDF_VERSION_MINOR >= 1)
#include "esp_netif.h"
#else
#include "tcpip_adapter.h"
#endif
#endif

void saveWifiSssidsInflash(void);
httpd_handle_t start_webserver(void);
void stop_webserver(httpd_handle_t server);

void mqtt_network_scan(void *parameters);
void mqtt_network_wifi(void *parameters);
void mqtt_network_list(void *parameters);

#endif
