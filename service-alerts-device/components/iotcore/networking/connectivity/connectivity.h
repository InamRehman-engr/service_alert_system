#ifndef connectivity_h_
#define connectivity_h_

#include "esp_err.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef CONFIG_ENABLE_WIFI
#include "wifi_manager.h"
#endif

#ifdef CONFIG_ENABLE_ETHERNET
#include "ethernet.h"
#endif

#ifdef CONFIG_ENABLE_MODEM
#include "modem.h"
#endif

#ifdef CONFIG_ENABLE_EPPP_CLIENT
#include "eppp_client.h"
#endif

#if defined(CONFIG_ENABLE_WIFI)
#if defined(CONFIG_ENABLE_ETHERNET) || defined(CONFIG_ENABLE_MODEM)
// No action required or an error can be added here if desired
#else
#define ALLOW_CUSTOM_INTERNET_AVAILABILITY_CHECK
#endif
#elif defined(CONFIG_ENABLE_ETHERNET)
#if defined(CONFIG_ENABLE_WIFI) || defined(CONFIG_ENABLE_MODEM)
// No action required or an error can be added here if desired
#else
#define ALLOW_CUSTOM_INTERNET_AVAILABILITY_CHECK
#endif
#elif defined(CONFIG_ENABLE_MODEM)
#if defined(CONFIG_ENABLE_WIFI) || defined(CONFIG_ENABLE_ETHERNET)
// No action required or an error can be added here if desired
#else
#define ALLOW_CUSTOM_INTERNET_AVAILABILITY_CHECK
#endif
#endif

typedef enum connectivity_network_provider_t {
  NETWORK_PROVIDER_WIFI,
  NETWORK_PROVIDER_ETHERNET,
  NETWORK_PROVIDER_MODEM,
  NETWORK_PROVIDER_EPPP_LINK,
  NETWORK_PROVIDER_MAX,
  NETWORK_PROVIDER_NONE
} connectivity_network_provider_t;

typedef enum connectivity_priority_type_t {
  NETWORK_PRIORITY_FIRST,
  NETWORK_PRIORITY_SECOND,
  NETWORK_PRIORITY_THIRD,
  NETWORK_PRIORITY_FOURTH
} connectivity_priority_type_t;

typedef struct {
  char description[32];
  char ip_address[16];
  char gateway[16];
  char netmask[16];
  char ssid[32];
  uint8_t connected;
} ip_info_t;

/*
    This is required from outside for priority.
    uint8_t priority[3] = {
        NETWORK_PROVIDER_WIFI,
        NETWORK_PROVIDER_NONE,
        NETWORK_PROVIDER_NONE
};

*/
// typedef enum network_mode_t {
//     NETWORK_MODE_AUTO,
//     NETWORK_MODE_MANUAL,
// };

// typedef enum network_control_t {
//     NETWORK_DISABLE_GSM,
//     NETWORK_ENABLE_GSM,

// };

/**
 * To use the custom internet provider checks it is required that only one
 * interface be used by the application. This makes sure that the application
 * will not lose internet when the device is changing internet providers.
 * Priority is just an array of
 *  uint8_t priority[3] = {
 *      NETWORK_PROVIDER_WIFI,
 *      NETWORK_PROVIDER_NONE,
 *      NETWORK_PROVIDER_NONE
 *  };
 */
void init_connectivity(
    uint8_t *priority,
    bool use_custom_internet_check_provider); // Pass null, false to initialize
                                              // with default ptiority and
                                              // internel internet checks

bool checkDeviceHasInternet();

void waitDeviceHasInternet();

void waitDeviceHasIP();

void getDeviceIP(ip_info_t *info);

#endif // __connectivity_h__
