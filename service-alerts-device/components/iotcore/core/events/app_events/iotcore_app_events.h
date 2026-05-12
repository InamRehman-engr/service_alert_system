#ifndef __iotcore_app_events_h__
#define __iotcore_app_events_h__

#include "esp_err.h"
#include "esp_event.h"
#include "esp_types.h"

// Event handler
extern esp_event_loop_handle_t iotcore_app_event_loop;
// Define event base
ESP_EVENT_DECLARE_BASE(IOTCORE_APP_EVENTS_BASE);

// Event IDs
typedef enum {
  // Connectivity
  INTERNET_CONNECTED_STATUS,
  INTERNET_PROVIDER_CHANGED,
  // Protocols
  MQTT_CONNECTIVITY_STATUS, // This will be emitted in case of connecion or
                            // disconnection of mqtt
  MQTT_DISCONNECT_COUNT_UPDATE,
  // Systen Info
  SYSTEM_BATTERY_STATUS_UPDATE,
  SYSTEM_RESET_COUNT,
  SYSTEM_RESET_REASON,
  SYSTEM_UPTIME_MS,
  SYSTEM_FREE_HEAP_BYTES,
  SYSTEM_IOTCORE_API_GOT_CLIENT_ID,
  // General OTA failure event
  GENERAL_OTA_FAILED_EVENT,
  // Native Ota Info
  NATIVE_OTA_EVENT,
  // Delta OTA Infor
  DELTA_OTA_EVENT,
  // HTTP Ota info
  HTTP_OTA_STATE_EVENT, // No data
} iotcore_app_events_t;

// Import data modals
#include "system_info_data_model.h"
// Start Event loop. Call this with ESP_ERR_CHECK
esp_err_t start_iotcore_app_event_loop();

// Public functions
esp_err_t post_iotcore_app_event(int32_t event_id, const void *event_data,
                                 size_t event_data_size);

/**
 * @brief
 *
 * @param event_id: Event ID identifies the event within comms event base
 * @param event_handler: Function that should run when an event is posted to a
 * loop
 * @param event_handler_arg: Data, aside from event data, that is passed to the
 * handler when it is called
 * @return esp_err_t
 */

esp_err_t register_iotcore_app_event(int32_t event_id,
                                     esp_event_handler_t event_handler,
                                     void *event_handler_arg);
#endif //__iotcore_app_events_h__