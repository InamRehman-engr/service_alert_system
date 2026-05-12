

#ifndef _http_server_apis_h_
#define _http_server_apis_h_

#include "cJSON.h"
#include <stdbool.h>
#include <stdint.h>

#include "http_rest_client.h"
#include "sysinfo.h"
#define NO_OF_DEVICES 8
#define DEVICE_TYPE_ID CONFIG_DEVICE_TYPE_ID

typedef struct __attribute__((__packed__)) {
  char *device_api_username;
  char *device_api_password;
  char *http_api_host;
} Http_credentials_t;

extern char clientID_get;
extern uint8_t api_cert_pem[6 * 1024];

/**
 * This function pointer will be used by the user to complete the onboarding
flow. like compare it with old id and restart the system and whatnot
 * Here is one way of usig it
 * void onboarding_Complete_cb(char * clientIDinNVS, char * clientIDfromServer)
{ if((strcmp(clientIDinNVS, clientIDfromServer) != 0)) { printf("myreset %s %s",
clientIDinNVS, clientIDfromServer); vTaskDelay(pdMS_TO_TICKS(1000));
        saveKeyValueInFlash_str("clientId", clientIDfromServer);
        esp_restart(); // This will allow us to restart a lot of things that
could not have been set without clientID.
        // This also allows us to change deviceIDs on server change
    }
    int deviceID = atoi(clientIDinNVS);
    post_iotcore_app_event(SYSTEM_IOTCORE_ONBOARDING_COMPLETE, &deviceID,
sizeof(int)); free(clientID); // Freed because of malloc from iotcore_init
}
*/
void iotcore_server_apis_task(Http_credentials_t http_variables,
                              device_info_t *onboardingData);
void print_unittest_label();
#endif