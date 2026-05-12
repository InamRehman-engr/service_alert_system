#ifndef __http_ota_handler_h__
#define __http_ota_handler_h__
#include "esp_types.h"

/**
 * Purpose of this file is to use the rest api client to download ota from url
 * and pass it to native_ota to be written to flash. This will only deal with
 * the writing part of it. Verification should be done elsewhere. Only boiler
 * plate this will setup is http and native ota
 */

typedef enum {
  HTTP_OTA_DOWNLOAD_SUCCESSFUL,
  HTTP_OTA_DOWNLOAD_STARTED,
  HTTP_OTA_DOWNLOAD_FAILED,
  HTTP_DELTA_OTA_GOT_HEADER,
} http_ota_state_t;
typedef enum {
  OTA_TYPE_NATIVE = 0,
  OTA_TYPE_DELTA,
} ota_type_t;
bool http_ota_handler_init(char *uri, bool delta);

#endif //__http_ota_handler_h__