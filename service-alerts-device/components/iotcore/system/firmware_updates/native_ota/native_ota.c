#include "native_ota.h"

#include "iotcore_events.h"

#define TAG "NATIVE_OTA"

static esp_ota_handle_t update_handle = 0;
ota_packet_t ota_packet;
size_t total_ota_size = 0;

TaskHandle_t ota_task_handle = NULL;
bool native_ota_in_progress = false;
native_ota_checksum_type_t native_ota_checksum_type = NATIVE_OTA_CHECKSUM_NONE;
QueueHandle_t ota_receive_queue = NULL;
mbedtls_sha256_context sha_ctx;
mbedtls_md5_context md5_ctx;

native_ota_failure_reason_t match_checksum(unsigned char *expected_checksum) {
  int ret = 0;
  unsigned char checksum_result[32];
  switch (native_ota_checksum_type) {
  case NATIVE_OTA_CHECKSUM_SHA256:
    mbedtls_sha256_finish(&sha_ctx, checksum_result);
    mbedtls_sha256_free(&sha_ctx);
    ret = memcmp(expected_checksum, checksum_result, 32)
              ? NATIVE_OTA_FAILED_CHECKSUM_MISMATCH
              : NATIVE_OTA_FAILURE_NONE;
    break;
  case NATIVE_OTA_CHECKSUM_MD5:
    mbedtls_md5_finish(&md5_ctx, checksum_result);
    mbedtls_md5_free(&md5_ctx);
    ret = memcmp(expected_checksum, checksum_result, 16)
              ? NATIVE_OTA_FAILED_CHECKSUM_MISMATCH
              : NATIVE_OTA_FAILURE_NONE;
    break;
  case NATIVE_OTA_CHECKSUM_NONE:
    ret = NATIVE_OTA_FAILURE_NONE;
  default:
    break;
  }
  return (native_ota_state_t)ret;
}
void calculate_checksum(const unsigned char *packet, size_t packet_length) {
  switch (native_ota_checksum_type) {
  case NATIVE_OTA_CHECKSUM_SHA256:
    mbedtls_sha256_update(&sha_ctx, packet, packet_length);
    break;
  case NATIVE_OTA_CHECKSUM_MD5:
    mbedtls_md5_update(&md5_ctx, packet, packet_length);
    break;
  default:
    break;
  }
}

/**
 * Send broadcast to system to update about ota progress
 */
int last_ota_update_percentage = 0;
void update_native_ota_progress(float ota_progress) {
  if (ota_progress - last_ota_update_percentage > 5 || ota_progress == 100) {
    post_iotcore_app_event(NATIVE_OTA_EVENT,
                           (void *)&(native_ota_event_message_t){
                               .progress = ota_progress,
                               .state = NATIVE_OTA_PROGRESS_UPDATE_EVENT,
                           },
                           sizeof(native_ota_event_message_t));
    last_ota_update_percentage = ota_progress;
    ESP_LOGI(TAG, "OTA Progress in native: %.2f%%", ota_progress);
  }
}

void update_native_ota_failure_state(native_ota_failure_reason_t reason) {
  last_ota_update_percentage = 0;
  if (reason == NATIVE_OTA_FAILURE_NONE)
    return;
  post_iotcore_app_event(NATIVE_OTA_EVENT,
                         (void *)&(native_ota_event_message_t){
                             .state = NATIVE_OTA_FAILED_EVENT,
                             .reason = reason,
                         },
                         sizeof(native_ota_event_message_t));
}

void update_native_ota_successful_state(void) {
  post_iotcore_app_event(NATIVE_OTA_EVENT,
                         (void *)&(native_ota_event_message_t){
                             .progress = 100.0f,
                             .state = NATIVE_OTA_SUCCESFUL_EVENT,
                         },
                         sizeof(native_ota_event_message_t));
  ESP_LOGI(TAG, "Native OTA success");
}

void native_ota_task(void *pvParams) {
  unsigned char *expected_checksum = pvParams;
  size_t received_size = 0;
  esp_err_t err = ESP_FAIL;
  float ota_progress = 0.0;
  while (native_ota_in_progress) {
    if (xQueueReceive(ota_receive_queue, &ota_packet, pdMS_TO_TICKS(10000)) ==
        pdTRUE) {
      err = esp_ota_write(update_handle, (const void *)ota_packet.packet,
                          ota_packet.packet_length);
      if (err != ESP_OK) {
        ESP_LOGE(TAG, "esp_ota_write failed (%s)!", esp_err_to_name(err));
        update_native_ota_failure_state(NATIVE_OTA_INVALID_PACKET);
        break;
      }
      calculate_checksum(ota_packet.packet, ota_packet.packet_length);
      free(ota_packet
               .packet); // Packet needs to be freed here for better memory
                         // management. This allows ota to work regardless of
                         // what size packet is recieved. If i check on the
                         // other side if the packet is used and then free it it
                         // will introduce more time delay in ota
      received_size += ota_packet.packet_length;
      ota_progress = ((float)received_size / (float)total_ota_size) * 100;
      update_native_ota_progress(ota_progress);
      if (ota_packet.last_packet) {
        if (match_checksum(expected_checksum) ==
            NATIVE_OTA_FAILED_CHECKSUM_MISMATCH) // need to give here expected
                                                 // checksum
        {
          ESP_LOGE(TAG, "Checksum failed");
          update_native_ota_failure_state(NATIVE_OTA_FAILED_CHECKSUM_MISMATCH);
          break;
        }
        err = esp_ota_end(update_handle);
        if (err != ESP_OK) {
          if (err == ESP_ERR_OTA_VALIDATE_FAILED) {
            ESP_LOGE(TAG, "Image validation failed, image is corrupted");
            update_native_ota_failure_state(
                NATIVE_OTA_INVALID_IMAGE); // invalid image or signature
                                           // verification failed due to
                                           // secureboot
            break;
          } else // other reason is ESP_ERR_INVALID_STATE
          {
            ESP_LOGE(TAG, "esp_ota_end failed (%s)!", esp_err_to_name(err));
            update_native_ota_failure_state(
                NATIVE_OTA_UNKNOWN_ERROR); // possibly flash encryption error
            break;
          }
        }
        err =
            esp_ota_set_boot_partition(esp_ota_get_next_update_partition(NULL));
        if (err != ESP_OK) {
          ESP_LOGE(TAG, "esp_ota_set_boot_partition failed (%s)!",
                   esp_err_to_name(err));
          update_native_ota_failure_state(NATIVE_OTA_SET_PARTITION_FAIL);
          break;
        }
        ESP_LOGI(TAG, "OTA Successful\nPrepare to restart system!");
        update_native_ota_successful_state();
        vTaskDelay(pdMS_TO_TICKS(
            2000)); // Give application time to do stuff before restart
        esp_restart();
      }
    }
  }
  native_ota_in_progress = false;
  ESP_LOGE(TAG, "Aborting OTA");
  total_ota_size = 0;
  if (ota_receive_queue != NULL)
    xQueueReset(ota_receive_queue);
  esp_ota_abort(update_handle);
  ota_task_handle = NULL;
  vTaskDelete(NULL);
}

/**
 * @return
 *    - ESP_OK: OTA operation commenced successfully.
 *    - ESP_ERR_INVALID_ARG: partition or out_handle arguments were NULL, or
 * partition doesn't point to an OTA app partition.
 *    - ESP_ERR_NO_MEM: Cannot allocate memory for OTA operation.
 *    - ESP_ERR_OTA_PARTITION_CONFLICT: Partition holds the currently running
 * firmware, cannot update in place.
 *    - ESP_ERR_NOT_FOUND: Partition argument not found in partition table.
 *    - ESP_ERR_OTA_SELECT_INFO_INVALID: The OTA data partition contains invalid
 * data.
 *    - ESP_ERR_INVALID_SIZE: Partition doesn't fit in configured flash size.
 *    - ESP_ERR_FLASH_OP_TIMEOUT or ESP_ERR_FLASH_OP_FAIL: Flash write failed.
 *    - ESP_ERR_OTA_ROLLBACK_INVALID_STATE: If the running app has not confirmed
 * state. Before performing an update, the application must be valid.
 *    - ESP_ERR_INVALID_STATE: Ota already in progress.
 */
esp_err_t initNativeOTA(size_t total_size,
                        native_ota_checksum_type_t checksum_type,
                        unsigned char *checksum) {
  if (native_ota_in_progress) {
    return ESP_ERR_INVALID_STATE;
  }
  total_ota_size = total_size;
  esp_err_t err = esp_ota_begin(esp_ota_get_next_update_partition(NULL),
                                OTA_WITH_SEQUENTIAL_WRITES, &update_handle);
  if (err != ESP_OK) {
    ESP_LOGE(TAG, "esp_ota_begin failed (%s)", esp_err_to_name(err));
    esp_ota_abort(update_handle);
  } else {
    ESP_LOGI(TAG, "esp_ota_begin succeeded");
  }
  if (ota_receive_queue == NULL) {
    ota_receive_queue = xQueueCreate(1, sizeof(ota_packet_t));
  }
  native_ota_in_progress = true;
  update_native_ota_progress(0.0f);
  native_ota_checksum_type = checksum_type;
  if (native_ota_checksum_type == NATIVE_OTA_CHECKSUM_SHA256) {
    mbedtls_sha256_init(&sha_ctx);
    mbedtls_sha256_starts(&sha_ctx, 0);
  } else if (native_ota_checksum_type == NATIVE_OTA_CHECKSUM_MD5) {
    mbedtls_md5_init(&md5_ctx);
    mbedtls_md5_starts(&md5_ctx);
  }
  ota_task_handle == NULL
      ? xTaskCreate(native_ota_task, "Native OTA Task", NATIVE_OTA_STACK,
                    checksum, CONFIG_NATIVE_OTA_TASK_PRIORITY, &ota_task_handle)
      : 0;
  return err;
}
void abortNativeOTA(void) {
  ESP_LOGW(TAG, "Abort Native OTA Callback Function");
  native_ota_in_progress = false;
  total_ota_size = 0;
  if (ota_task_handle != NULL) {
    vTaskDelete(ota_task_handle);
    ota_task_handle = NULL;
  }
  update_native_ota_failure_state(NATIVE_OTA_CANCELLED);
  if (ota_receive_queue != NULL)
    xQueueReset(ota_receive_queue);
}

native_ota_t native_ota = {.init_native_ota = initNativeOTA,
                           .dataqueue = &ota_receive_queue,
                           .abort_native_ota = abortNativeOTA};