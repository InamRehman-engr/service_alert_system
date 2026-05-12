#include "iotcore_app_events.h"

ESP_EVENT_DEFINE_BASE(IOTCORE_APP_EVENTS_BASE);

esp_event_loop_handle_t iotcore_app_event_loop;
esp_err_t start_iotcore_app_event_loop() {
  return esp_event_loop_create(
      &(esp_event_loop_args_t){
          .queue_size = CONFIG_IOTCORE_EVENT_QUEUE_SIZE,
          .task_name = "iotcore_app_event_loop",
          .task_priority = CONFIG_IOTCORE_EVENT_LOOP_TASK_PRIORITY,
          .task_stack_size = CONFIG_IOTCORE_APP_EVENT_HEAP_SIZE,
      },
      &iotcore_app_event_loop);
}

esp_err_t post_iotcore_app_event(int32_t event_id, const void *event_data,
                                 size_t event_data_size) {
  return esp_event_post_to(iotcore_app_event_loop, IOTCORE_APP_EVENTS_BASE,
                           event_id, event_data, event_data_size,
                           pdMS_TO_TICKS(100));
}

esp_err_t register_iotcore_app_event(int32_t event_id,
                                     esp_event_handler_t event_handler,
                                     void *event_handler_arg) {
  return esp_event_handler_register_with(iotcore_app_event_loop,
                                         IOTCORE_APP_EVENTS_BASE, event_id,
                                         event_handler, event_handler_arg);
}
