#ifndef __app_iotcore_mqtt_events_h__
#define __app_iotcore_mqtt_events_h__

// clang-format off
#include "stdint.h"
// clang-format on
#include "esp_event_base.h"
#include "esp_types.h"

void mqtt_pub_sub_event_handler(void *event_handler_arg,
                                esp_event_base_t event_base, int32_t event_id,
                                void *event_data);

#endif // __app_iotcore_mqtt_events_h__