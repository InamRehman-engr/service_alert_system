#include "http_ota_handler.h"

#include "esp_log.h"

#include "base64_decoding.h"
#include "delta_ota.h"
#include "http_rest_client.h"
#include "iotcore_events.h"
#include "native_ota.h"

bool ota_in_progress = false;
int ota_total_size = 0;
int ota_done_size = 0;
unsigned char *decodedMd5 = NULL;
native_ota_failure_reason_t native_ota_state = NATIVE_OTA_FAILURE_NONE;
delta_ota_failure_reason_t delta_ota_state = DELTA_OTA_FAILURE_NONE;
#define MAX_NUMBER_OF_HTTPS_OTA_RETRIES CONFIG_MAX_OTA_RETRIES

static const char *TAG = "http_ota_handler";
ota_type_t current_ota_type;

void update_http_ota_state(http_ota_state_t state) {
  post_iotcore_app_event(HTTP_OTA_STATE_EVENT, &state,
                         sizeof(http_ota_state_t));
}

esp_err_t http_ota_event_handler(esp_http_client_event_t *evt) {
  if (native_ota_state != NATIVE_OTA_FAILURE_NONE ||
      delta_ota_state != DELTA_OTA_FAILURE_NONE) {
    // ota failed due to some reason, abort http request
    ota_total_size = 0;
    ota_done_size = 0;
    native_ota_state = NATIVE_OTA_FAILURE_NONE;
    delta_ota_state = DELTA_OTA_FAILURE_NONE;
    esp_http_client_close(evt->client);
    return ESP_FAIL;
  }
  switch (evt->event_id) {
  // Unused cases
  case HTTP_EVENT_ERROR:
  case HTTP_EVENT_ON_CONNECTED:
  case HTTP_EVENT_HEADER_SENT:
  case HTTP_EVENT_ON_FINISH:
  case HTTP_EVENT_DISCONNECTED:
  case HTTP_EVENT_REDIRECT:
    break;
  case HTTP_EVENT_ON_HEADER:
    if (strcasecmp(evt->header_key, "Content-Length") == 0) {
      if (ota_total_size == 0) {
        ota_done_size = 0;
        ota_total_size = atoi(evt->header_value);
      } else {
        if (ota_total_size == atoi(evt->header_value)) {
          // Called for resume but received full package. Abort and start again.
          ESP_LOGE(TAG, "Received full package. Start from scratch again");
          ota_done_size = 0;
          ota_total_size = atoi(evt->header_value);
          current_ota_type == OTA_TYPE_NATIVE ? native_ota.abort_native_ota()
                                              : delta_ota.abort_delta_ota();
        } else if (ota_total_size ==
                   (ota_done_size + atoi(evt->header_value))) {
          // Resume packets
        }
      }
    }
    if (strcmp(evt->header_key, "Content-MD5") == 0) {
      size_t decoded_length;
      decodedMd5 =
          base64_decode(evt->header_value, strlen(evt->header_value),
                        &decoded_length); // Need to free this after use
    }

    break;
  case HTTP_EVENT_ON_DATA:
    if (ota_total_size != 0) {
      if (decodedMd5 != 0) {
        current_ota_type == OTA_TYPE_NATIVE
            ? native_ota.init_native_ota(ota_total_size,
                                         NATIVE_OTA_CHECKSUM_MD5, decodedMd5)
            : delta_ota.init_delta_ota(ota_total_size, DELTA_OTA_CHECKSUM_MD5,
                                       decodedMd5);
      } else {
        current_ota_type == OTA_TYPE_NATIVE
            ? native_ota.init_native_ota(ota_total_size,
                                         NATIVE_OTA_CHECKSUM_NONE, decodedMd5)
            : delta_ota.init_delta_ota(ota_total_size, NATIVE_OTA_CHECKSUM_NONE,
                                       decodedMd5);
      }
    }
    ota_done_size += evt->data_len;
    ota_done_size == evt->data_len
        ? update_http_ota_state(HTTP_OTA_DOWNLOAD_STARTED)
        : 0;
    ota_done_size == ota_total_size
        ? update_http_ota_state(HTTP_OTA_DOWNLOAD_SUCCESSFUL)
        : 0;
    ota_packet_t ota_data = {.last_packet = ota_total_size == ota_done_size,
                             .packet_length = evt->data_len,
                             .packet = malloc(evt->data_len * sizeof(char))};
    memcpy(ota_data.packet, evt->data, evt->data_len);
    current_ota_type == OTA_TYPE_NATIVE
        ? *native_ota.dataqueue == NULL
              ? free(ota_data.packet)
              : xQueueSend(*native_ota.dataqueue, &ota_data,
                           pdMS_TO_TICKS(5000))
    : *delta_ota.dataqueue == NULL
        ? free(ota_data.packet)
        : xQueueSend(*delta_ota.dataqueue, &ota_data, pdMS_TO_TICKS(5000));
    break;
  }
  return ESP_OK;
}

void http_ota_handler_task(void *pvParameters) {
  ota_in_progress = true;
  char *uri = (char *)pvParameters;
  int download_status_code = 0;
  for (int i = 0; i < MAX_NUMBER_OF_HTTPS_OTA_RETRIES; i++) {
    char *value;
    asprintf(&value, "bytes=%d-%d", ota_done_size, ota_total_size);
    http_rest_recv_buffer_t http_rest_recv_buffer; // Response buffer.
    http_header_t headers[] = {
        {.key = "Range", .value = value},
    };
    http_rest_client_get_custom_handler(
        uri, &http_rest_recv_buffer, http_ota_event_handler,
        ota_done_size ? headers : NULL,
        ota_done_size ? sizeof(headers) / sizeof(http_header_t) : 0);
    download_status_code = http_rest_recv_buffer.status_code;
    http_rest_client_cleanup(&http_rest_recv_buffer);
    free(value);
    vTaskDelay(pdMS_TO_TICKS(5000));
    if (ota_done_size == ota_total_size && ota_done_size != 0) {
      // download completed here, break if there is no ota failure present
      if (native_ota_state == NATIVE_OTA_FAILURE_NONE &&
          delta_ota_state == DELTA_OTA_FAILURE_NONE)
        break;
      else {
        ota_total_size = 0;
        ota_done_size = 0;
        native_ota_state = NATIVE_OTA_FAILURE_NONE;
        delta_ota_state = DELTA_OTA_FAILURE_NONE;
      }
    }
  }
  if (download_status_code != 200 &&
      download_status_code != 206) // neither success and nor partial download
  {
    ESP_LOGE(TAG, "Download Failed. Status Code: %d", download_status_code);
    update_http_ota_state(HTTP_OTA_DOWNLOAD_FAILED);
    current_ota_type == OTA_TYPE_NATIVE ? native_ota.abort_native_ota()
                                        : delta_ota.abort_delta_ota();
  }
  ota_in_progress = false;
  ota_total_size = 0;
  ota_done_size = 0;
  native_ota_state = NATIVE_OTA_FAILURE_NONE;
  delta_ota_state = DELTA_OTA_FAILURE_NONE;
  free(uri);
  vTaskDelete(NULL);
}

void native_ota_status_event_handler_to_http(void *event_handler_arg,
                                             esp_event_base_t event_base,
                                             int32_t event_id,
                                             void *event_data) {
  native_ota_event_message_t native_ota_event_message =
      *(native_ota_event_message_t *)event_data;
  if (native_ota_event_message.state == NATIVE_OTA_FAILED_EVENT) {
    if (native_ota_event_message.reason != NATIVE_OTA_FAILURE_NONE) {
      native_ota_state = native_ota_event_message.reason;
      ESP_LOGE(TAG, "Native OTA Failed. Reason: %d", native_ota_state);
    }
  }
}
void delta_ota_status_event_handler_to_http(void *event_handler_arg,
                                            esp_event_base_t event_base,
                                            int32_t event_id,
                                            void *event_data) {
  delta_ota_event_message_t delta_ota_event_message =
      *(delta_ota_event_message_t *)event_data;
  if (delta_ota_event_message.state == DELTA_OTA_FAILED_EVENT) {
    if (delta_ota_event_message.reason != DELTA_OTA_FAILURE_NONE) {
      delta_ota_state = delta_ota_event_message.reason;
      ESP_LOGE(TAG, "Delta OTA Failed. Reason: %d", delta_ota_state);
    }
  }
}

bool http_ota_handler_init(char *uri, bool delta) {
  ESP_LOGE(TAG, "OTA_HANDLER_INIT starting memory: %ld",
           esp_get_free_heap_size());
  /**
   * Required memory is 5364 according to uxTaskGetStackHighWaterMark
   */
  if (ota_in_progress) {
    ESP_LOGW(TAG, "Current OTA In Progress. Can not start.");
    return false;
  }
  ota_total_size = 0;
  ota_done_size = 0;
  native_ota_state = NATIVE_OTA_FAILURE_NONE;
  delta_ota_state = DELTA_OTA_FAILURE_NONE;
  current_ota_type = delta ? OTA_TYPE_DELTA : OTA_TYPE_NATIVE;
  char *uri_local = malloc(strlen(uri) + 1);
  snprintf(uri_local, strlen(uri) + 1, "%s", uri);
  register_iotcore_app_event(NATIVE_OTA_EVENT,
                             native_ota_status_event_handler_to_http, NULL);
  register_iotcore_app_event(DELTA_OTA_EVENT,
                             delta_ota_status_event_handler_to_http, NULL);
  if (xTaskCreate(http_ota_handler_task, "http_ota_handler_task", 1024 * 4,
                  (void *)uri_local, CONFIG_HTTP_FILE_DOWNLOAD_TASK_PRIORITY,
                  NULL) == pdTRUE) {
    return true;
  } else {
    free(uri_local);
    return false;
  }
}