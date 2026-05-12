#ifndef __app_iotcore_ota_event_handler_h__
#define __app_iotcore_ota_event_handler_h__

#include "esp_types.h"

void init_ota_event_handler(char *topic_to_send_ota_alerts, char *newVersion);
#endif // __app_iotcore_ota_event_handler_h__