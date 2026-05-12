#ifndef _wifi_manager_h_
#define _wifi_manager_h_

/*
    What should be contained in a wifi manager.
    1. Connections list like wpa_supplicant. In this case a list of connections.
    2. A scan list
    3. Ability to filter out which connections can connect to.
    4. Retry logic for reconnection to specified ap
    5. AP mode
    6. Configurable fallback or always on AP


    These are the things that would allow it to be interfaced with the outside
   connectivity.
    1. Init/deinit wifi manager
    2. Provide/Update Wifi list
    3. Grep status.
*/

#include "cJSON.h"
#include "esp_eap_client.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "freertos/queue.h"
#include "freertos/semphr.h"
#include "freertos/task.h"
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define WIFI_STA_SCAN_INTERVAL 30000

#if CONFIG_WIFI_POWER_SAVE_MIN_MODEM
#define DEFAULT_PS_MODE WIFI_PS_MIN_MODEM
#elif CONFIG_WIFI_POWER_SAVE_MAX_MODEM
#define DEFAULT_PS_MODE WIFI_PS_MAX_MODEM
#elif CONFIG_WIFI_POWER_SAVE_NONE
#define DEFAULT_PS_MODE WIFI_PS_NONE
#else
#define DEFAULT_PS_MODE WIFI_PS_MIN_MODEM
#endif /*CONFIG_POWER_SAVE_MODEM*/

typedef struct {
  char ssid[40];
  char password[40];
  uint8_t priority;
} wifi_ssid_t;

void stop_wifi_manager();
void start_wifi_manager();
void init_wifi_manager();
void rotateWifiList();
void updateWifiList(cJSON *ssids_json);

void device_connected_to_ap_cb();
void device_disconnected_from_ap_cb();
#endif