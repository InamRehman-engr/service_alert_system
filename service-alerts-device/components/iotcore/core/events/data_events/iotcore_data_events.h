#ifndef __iotcore_data_events_h__
#define __iotcore_data_events_h__

#include "esp_err.h"
#include "esp_event.h"
#include "esp_types.h"

// Event handler
extern esp_event_loop_handle_t iotcore_data_event_loop;
// Define event base
ESP_EVENT_DECLARE_BASE(IOTCORE_DATA_EVENTS_BASE);

// Event IDs
typedef enum {
  MQTT_DATA_PUBLISH, // This can be used to publish messages using mqtt. This is
                     // here to remove mqtt publish from every place
  MQTT_SUBSCRIBE_TO_TOPIC,
  /// TODO: Add subscribe somehow. but that would also make the public subscribe
  MQTT_UNSUBSCRIBE_FROM_TOPIC, // This can be used to unsubscribe from a topic.
  /// calls useless. tradeof
  MQTT_ON_CONNECT_PUBLICATION,
} iotcore_data_events_t;

// Import data modals
#include "mqtt_data_model.h"

// Start Event loop. Call this with ESP_ERR_CHECK
esp_err_t start_iotcore_data_event_loop();

// Delete event loop

// Public functions
esp_err_t post_iotcore_data_event(int32_t event_id, const void *event_data,
                                  size_t event_data_size);

/**
 * @brief
 *
 * @param event_id: Event ID identifies the event within data event base
 * @param event_handler: Function that should run when an event is posted to a
 * loop
 * @param event_handler_arg: Data, aside from event data, that is passed to the
 * handler when it is called
 * @return esp_err_t
 */
esp_err_t register_iotcore_data_event(int32_t event_id,
                                      esp_event_handler_t event_handler,
                                      void *event_handler_arg);
#endif //__iotcore_data_events_h__