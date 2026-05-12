#include "delta_ota.h"
#include "esp_app_format.h"
#include "esp_delta_ota.h"
#include "esp_err.h"
#include "esp_log.h"
#include "esp_ota_ops.h"
#include "iotcore_events.h"
#include "mbedtls/md5.h"
#include "mbedtls/sha256.h"

#define TAG "DELTA_OTA"

#define IMG_HEADER_LEN sizeof(esp_image_header_t)
#define DELTA_IMAGE_HEADER_LEN 64

static esp_ota_handle_t update_handle = 0;
delta_ota_packet_t delta_ota_packet;
size_t total_delta_ota_size = 0;
const esp_partition_t *current_partition;
const esp_partition_t *destination_partition;
esp_delta_ota_handle_t handle = NULL;
bool delta_ota_image_validated = false;
static esp_err_t delta_ota_read_cb(uint8_t *buf_p, size_t size,
                                   int src_offset) {
  if (size <= 0) {
    return ESP_ERR_INVALID_ARG;
  }
  return esp_partition_read(current_partition, src_offset, buf_p, size);
}
static bool verify_chip_id(void *bin_header_data) {
  esp_image_header_t *header = (esp_image_header_t *)bin_header_data;
  if (header->chip_id != CONFIG_IDF_FIRMWARE_CHIP_ID) {
    ESP_LOGE(TAG, "Mismatch chip id, expected %d, found %d",
             CONFIG_IDF_FIRMWARE_CHIP_ID, header->chip_id);
    return false;
  }
  return true;
}
#if (ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(5, 2, 0))
static esp_err_t delta_ota_write_cb(const uint8_t *buf_p, size_t size,
                                    void *user_data)
#else
static esp_err_t delta_ota_write_cb(const uint8_t *buf_p, size_t size)
#endif
{
  if (size <= 0) {
    return ESP_ERR_INVALID_ARG;
  }

  static char header_data[IMG_HEADER_LEN];
  static bool chip_id_verified = false;
  static int header_data_read = 0;
  int index = 0;

  if (!chip_id_verified) {
    if (header_data_read + size <= IMG_HEADER_LEN) {
      memcpy(header_data + header_data_read, buf_p, size);
      header_data_read += size;
      return ESP_OK;
    } else {
      index = IMG_HEADER_LEN - header_data_read;
      memcpy(header_data + header_data_read, buf_p, index);

      if (!verify_chip_id(header_data)) {
        return ESP_ERR_INVALID_VERSION;
      }
      chip_id_verified = true;

      // Write data in header_data buffer.
      esp_err_t err = esp_ota_write(update_handle, header_data, IMG_HEADER_LEN);
      if (err != ESP_OK) {
        return err;
      }
    }
  }
  return esp_ota_write(update_handle, buf_p + index, size - index);
}
esp_delta_ota_cfg_t config = {
    .read_cb = &delta_ota_read_cb,
#if (ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(5, 2, 0))
    .user_data = "https_delta_ota",
    .write_cb_with_user_data = &delta_ota_write_cb,
#else
    .write_cb = &delta_ota_write_cb,
#endif
};

TaskHandle_t delta_ota_task_handle = NULL;
static bool delta_ota_in_progress = false;
delta_ota_checksum_type_t delta_ota_checksum_type = DELTA_OTA_CHECKSUM_SHA256;
static mbedtls_sha256_context sha_ctx;
static mbedtls_md5_context md5_ctx;
QueueHandle_t delta_ota_receive_queue = NULL;
delta_ota_failure_reason_t
match_checksum_delta(unsigned char *expected_checksum) {
  int ret = 0;
  unsigned char checksum_result[32];
  switch (delta_ota_checksum_type) {
  case DELTA_OTA_CHECKSUM_SHA256:
    mbedtls_sha256_finish(&sha_ctx, checksum_result);
    mbedtls_sha256_free(&sha_ctx);
    ret = memcmp(expected_checksum, checksum_result, 32)
              ? DELTA_OTA_FAILED_CHECKSUM_MISMATCH
              : DELTA_OTA_FAILURE_NONE;
    break;
  case DELTA_OTA_CHECKSUM_MD5:
    mbedtls_md5_finish(&md5_ctx, checksum_result);
    mbedtls_md5_free(&md5_ctx);
    ret = memcmp(expected_checksum, checksum_result, 16)
              ? DELTA_OTA_FAILED_CHECKSUM_MISMATCH
              : DELTA_OTA_FAILURE_NONE;
    break;
  case DELTA_OTA_CHECKSUM_NONE:
    ret = DELTA_OTA_FAILURE_NONE;
  default:
    break;
  }
  return (delta_ota_state_t)ret;
}

void calculate_ota_checksum(const unsigned char *packet, size_t packet_length) {
  switch (delta_ota_checksum_type) {
  case DELTA_OTA_CHECKSUM_SHA256:
    mbedtls_sha256_update(&sha_ctx, packet, packet_length);
    break;
  case DELTA_OTA_CHECKSUM_MD5:
    mbedtls_md5_update(&md5_ctx, packet, packet_length);
    break;
  case DELTA_OTA_CHECKSUM_NONE:
    break;
  default:
    ESP_LOGE(TAG, "Invalid checksum type");
    break;
  }
}
int last_delta_ota_update_percentage = 0;
void update_delta_ota_progress(float ota_progress) {
  if (ota_progress - last_delta_ota_update_percentage > 5 ||
      ota_progress == 100) {
    post_iotcore_app_event(DELTA_OTA_EVENT,
                           (void *)&(delta_ota_event_message_t){
                               .progress = ota_progress,
                               .state = DELTA_OTA_PROGRESS_UPDATE_EVENT,
                           },
                           sizeof(delta_ota_event_message_t));
    last_delta_ota_update_percentage = ota_progress;
    ESP_LOGI(TAG, "OTA Progress in delta: %.2f%%", ota_progress);
  }
}
void update_delta_ota_failure_state(delta_ota_failure_reason_t reason) {
  last_delta_ota_update_percentage = 0;
  if (reason == DELTA_OTA_FAILURE_NONE)
    return;
  post_iotcore_app_event(DELTA_OTA_EVENT,
                         (void *)&(delta_ota_event_message_t){
                             .state = DELTA_OTA_FAILED_EVENT,
                             .reason = reason,
                         },
                         sizeof(delta_ota_event_message_t));
}
void update_delta_ota_successful_state(void) {
  post_iotcore_app_event(DELTA_OTA_EVENT,
                         (void *)&(delta_ota_event_message_t){
                             .progress = 100.0f,
                             .state = DELTA_OTA_SUCCESFUL_EVENT,
                         },
                         sizeof(delta_ota_event_message_t));
  ESP_LOGI(TAG, "Delta OTA success");
}

delta_ota_image_failure_reason_t
delta_ota_header_image_validation(void *image_header_image) {
  if (!image_header_image) {
    return DELTA_OTA_IMAGE_NULL_HEADER;
  }

  uint32_t recv_magic = *(uint32_t *)image_header_image;
  uint8_t *digest = (uint8_t *)(image_header_image + 4);
  if (recv_magic != CONFIG_DELTA_MAGIC) {
    ESP_LOGE(TAG, "Invalid magic word in patch");
    return DELTA_OTA_IMAGE_INVALID_MAGIC_WORD;
  }
  /// TODO: add support for other checksum types
  uint8_t sha_256[32] = {0};
  esp_partition_get_sha256(esp_ota_get_running_partition(), sha_256);
  if (memcmp(sha_256, digest, 32) != 0) {
    ESP_LOGE(TAG, "SHA256 of current firmware differs from than in patch "
                  "header. Invalid patch for current firmware");
    return DELTA_OTA_IMAGE_INVALID_CHECKSUM;
  }
  return DELTA_OTA_IMAGE_VALID;
}
void delta_ota_task(void *pvParams) {
  unsigned char *expected_checksum = pvParams;
  size_t received_size = 0;
  esp_err_t err = ESP_FAIL;
  float ota_progress = 0.0;
  while (delta_ota_in_progress) {
    if (xQueueReceive(delta_ota_receive_queue, &delta_ota_packet,
                      pdMS_TO_TICKS(2000)) == pdTRUE) {
      if (!ota_progress) {
        delta_ota_image_failure_reason_t img_err =
            delta_ota_header_image_validation(delta_ota_packet.packet);
        if (img_err != DELTA_OTA_IMAGE_VALID) {
          ESP_LOGE(TAG, "Received invalid image");
          delta_ota_image_validated = false;
          delta_ota_in_progress = false;
          update_delta_ota_failure_state(DELTA_OTA_FAILED_CHECKSUM_MISMATCH);
          free(delta_ota_packet.packet);
          break;
        } else {
          delta_ota_image_validated = true;
          ESP_LOGI(TAG, "Received valid image");
          // Calculate the new size of the packet
          size_t temp_packet_size =
              delta_ota_packet.packet_length - DELTA_IMAGE_HEADER_LEN;
          // Allocate memory for the new packet
          uint8_t *temp_packet = malloc(temp_packet_size);
          memcpy(temp_packet, delta_ota_packet.packet + DELTA_IMAGE_HEADER_LEN,
                 temp_packet_size);
          free(delta_ota_packet.packet);
          delta_ota_packet.packet = temp_packet;
          delta_ota_packet.packet_length = temp_packet_size;
        }
      }
      err = esp_delta_ota_feed_patch(handle,
                                     (const uint8_t *)delta_ota_packet.packet,
                                     delta_ota_packet.packet_length);
      calculate_ota_checksum(delta_ota_packet.packet,
                             delta_ota_packet.packet_length);
      free(delta_ota_packet
               .packet); // Packet needs to be freed here for better memory
                         // management. This allows ota to work regardless of
                         // what size packet is recieved. If i check on the
                         // other side if the packet is used and then free it it
                         // will introduce more time delay in ota
      if (err != ESP_OK) {
        ESP_LOGE(TAG, "esp_delta_ota_feed_patch failed (%s)!",
                 esp_err_to_name(err));
        update_delta_ota_failure_state(DELTA_OTA_INVALID_PACKET);
        break;
      }

      received_size += delta_ota_packet.packet_length;
      ota_progress = ((float)received_size / (float)total_delta_ota_size) * 100;
      update_delta_ota_progress(ota_progress);
      if (delta_ota_packet.last_packet) {
        err = esp_delta_ota_finalize(handle);
        if (err != ESP_OK) {
          ESP_LOGE(TAG, "esp_delta_ota_finalize() failed : %s",
                   esp_err_to_name(err));
          update_delta_ota_failure_state(DELTA_OTA_FAILED_FINALIZE);
          break;
        }
        if (match_checksum_delta(expected_checksum) ==
            DELTA_OTA_FAILED_CHECKSUM_MISMATCH) {
          ESP_LOGE(TAG, "Checksum failed");
          update_delta_ota_failure_state(DELTA_OTA_FAILED_CHECKSUM_MISMATCH);
          break;
        }
        err = esp_ota_end(update_handle);
        if (err != ESP_OK) {
          if (err == ESP_ERR_OTA_VALIDATE_FAILED) {
            ESP_LOGE(TAG, "Image validation failed, image is corrupted");
            update_delta_ota_failure_state(
                DELTA_OTA_INVALID_IMAGE); // invalid image or signature
                                          // verification failed due to
                                          // secureboot
            break;
          } else // other reason is ESP_ERR_INVALID_STATE
          {
            ESP_LOGE(TAG, "esp_ota_end failed (%s)!", esp_err_to_name(err));
            update_delta_ota_failure_state(
                DELTA_OTA_UNKNOWN_ERROR); // possibly flash encryption error
            break;
          }
        }
        err =
            esp_ota_set_boot_partition(esp_ota_get_next_update_partition(NULL));
        if (err != ESP_OK) {
          ESP_LOGE(TAG, "esp_ota_set_boot_partition failed (%s)!",
                   esp_err_to_name(err));
          update_delta_ota_failure_state(DELTA_OTA_SET_PARTITION_FAIL);
          break;
        }
        ESP_LOGI(TAG, "OTA Successful\nPrepare to restart system!");
        update_delta_ota_successful_state();
        vTaskDelay(pdMS_TO_TICKS(
            2000)); // Give application time to do stuff before restart
        esp_restart();
      }
    }
  }
  delta_ota_in_progress = false;
  delta_ota_image_validated = false;
  total_delta_ota_size = 0;
  ESP_LOGE(TAG, "DELTA OTA Task Aborted");
  if (delta_ota_receive_queue != NULL)
    xQueueReset(delta_ota_receive_queue);
  esp_ota_abort(update_handle);
  delta_ota_task_handle = NULL;
  vTaskDelete(NULL);
}

esp_err_t init_delta_ota(size_t total_size,
                         delta_ota_checksum_type_t checksum_type,
                         unsigned char *checksum) {
  if (delta_ota_in_progress) {
    return ESP_ERR_INVALID_STATE;
  }
  ESP_LOGI(TAG, "Initializing DELTA OTA");
  delta_ota_image_validated = false;
  total_delta_ota_size = total_size;
  current_partition = esp_ota_get_running_partition();
  destination_partition = esp_ota_get_next_update_partition(NULL);
  esp_err_t err =
      esp_ota_begin(destination_partition, OTA_SIZE_UNKNOWN, &update_handle);
  if (err != ESP_OK) {
    ESP_LOGE(TAG, "esp_ota_begin failed (%s)", esp_err_to_name(err));
    esp_ota_abort(update_handle);
    return err;
  } else {
    ESP_LOGI(TAG, "esp_ota_begin succeeded");
  }
  if (delta_ota_receive_queue == NULL) {
    delta_ota_receive_queue = xQueueCreate(1, sizeof(delta_ota_packet_t));
  }
  if (handle == NULL) {
    handle = esp_delta_ota_init(&config);
  }
  delta_ota_in_progress = true;
  update_delta_ota_progress(0.0f);
  delta_ota_checksum_type = checksum_type;
  if (delta_ota_checksum_type == DELTA_OTA_CHECKSUM_SHA256) {
    mbedtls_sha256_init(&sha_ctx);
    mbedtls_sha256_starts(&sha_ctx, 0);
  } else if (delta_ota_checksum_type == DELTA_OTA_CHECKSUM_MD5) {
    mbedtls_md5_init(&md5_ctx);
    mbedtls_md5_starts(&md5_ctx);
  }
  delta_ota_task_handle == NULL
      ? xTaskCreate(delta_ota_task, "DELTA OTA Task", DELTA_OTA_STACK, checksum,
                    configMAX_PRIORITIES - 2, &delta_ota_task_handle)
      : 0;
  ESP_LOGI(TAG, "DELTA OTA Task Created Successfully");

  return err;
}
void abortDeltaOTA(void) {
  ESP_LOGW(TAG, "Abort Delta OTA function Callback");
  delta_ota_in_progress = false;
  total_delta_ota_size = 0;
  if (delta_ota_task_handle != NULL) {
    vTaskDelete(delta_ota_task_handle);
    delta_ota_task_handle = NULL;
  }
  update_delta_ota_failure_state(DELTA_OTA_CANCELLED);
  if (!delta_ota_receive_queue)
    xQueueReset(delta_ota_receive_queue);
}

delta_ota_t delta_ota = {.init_delta_ota = init_delta_ota,
                         .dataqueue = &delta_ota_receive_queue,
                         .abort_delta_ota = abortDeltaOTA};