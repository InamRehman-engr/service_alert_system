#include "iotcore_events.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <ctype.h>
#include <stdlib.h>
#include <string.h>

static const char *TAG = "iotcore_events";
void start_iotcore_event_loop() {
  ESP_ERROR_CHECK(start_iotcore_app_event_loop());
  ESP_ERROR_CHECK(start_iotcore_error_event_loop());
  ESP_ERROR_CHECK(start_iotcore_data_event_loop());
  ESP_LOGI(TAG, "iotcore event loop started");
}

void delete_iotcore_event_loop() {
  esp_event_loop_delete(iotcore_app_event_loop);
  esp_event_loop_delete(iotcore_error_event_loop);
  esp_event_loop_delete(iotcore_data_event_loop);
  ESP_LOGW(TAG, "iotcore event loop deleted");
}