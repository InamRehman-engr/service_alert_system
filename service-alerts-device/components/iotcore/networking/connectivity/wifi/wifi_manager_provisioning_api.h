#ifndef __wifi_manager_provisioning_api_h__
#define __wifi_manager_provisioning_api_h__
#include "wifi_manager.h"

/**
 * This will return the already utilized scan list. dont modify the list.
 */
wifi_ap_record_t *api_wifi_manager_get_scan_list(uint8_t *number);

/**
 * Make an object of the wifi and return it in here to connect to it.
 *
 * Using this method will discard the already initialized list and add only 2
 * wifi's to the list. This one and Iotcore Test at 2nd priority. Connection
 * will take some time as it is done from the default loop
 */
void api_wifi_manager_connect_to_wifi(char *ssid, char *password);

/**
 * Update known network list
 */
void api_wifi_manager_update_known_network_list(cJSON *list);

/**
 * AP provisioning mode. this can be used to perform provisioning if internet is
 * not available.
 */
void api_wifi_manager_provisioning_mode(bool mode);
#endif // __wifi_manager_provisioning_api_h__