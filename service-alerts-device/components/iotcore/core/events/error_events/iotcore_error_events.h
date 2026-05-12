#ifndef __iotcore_error_events_h__
#define __iotcore_error_events_h__

#include "esp_err.h"
#include "esp_event.h"
#include "esp_types.h"

// Event handler
extern esp_event_loop_handle_t iotcore_error_event_loop;
// Define event base
ESP_EVENT_DECLARE_BASE(IOTCORE_ERROR_EVENTS_BASE);

// Event IDs
typedef enum {
  // System Errors
  TASK_CREATE_FAILED,
  THREAD_FAILED,
  GENERAL_ESP_ERROR_CODE, // This will contain events whose data is int castable
  TASK_STACK_OVERFLOW,
  MEMORY_ALLOCATION_FAILED,

  OTA_FAILED

} iotcore_error_events_t;

// Import data modals
#include "iotcore_errors_data_model.h"

// Start Event loop. Call this with ESP_ERR_CHECK
esp_err_t start_iotcore_error_event_loop();

esp_err_t post_iotcore_error_event(int32_t event_id, const void *event_data,
                                   size_t event_data_size);

/**
 * @brief
 *
 * @param event_id: Event ID identifies the event within error event base
 * @param event_handler: Function that should run when an event is posted to a
 * loop
 * @param event_handler_arg: Data, aside from event data, that is passed to the
 * handler when it is called
 * @return esp_err_t
 */
esp_err_t register_iotcore_error_event(int32_t event_id,
                                       esp_event_handler_t event_handler,
                                       void *event_handler_arg);
#endif //__iotcore_error_events_h__