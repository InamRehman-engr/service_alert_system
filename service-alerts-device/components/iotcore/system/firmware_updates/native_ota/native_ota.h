#pragma once
#include "esp_app_format.h"
#include "esp_err.h"
#include "esp_flash_partitions.h"
#include "esp_log.h"
#include "esp_ota_ops.h"
#include "esp_system.h"
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "freertos/queue.h"
#include "freertos/semphr.h"
#include "freertos/task.h"
#include "mbedtls/md5.h"
#include "mbedtls/sha256.h"
#include "sdkconfig.h"
#include "string.h"
#include <stdbool.h>
#include <stdint.h>

/// TODO: Add checksum verification for both sha256 and md5
/**
 * Native OTA component
 * This component will serve as the most basic way of writing to flash. This
 * will allow it to be used with packets from anywhere. even i2c or uart. This
 * will open up more ways to do ota.
 *
 * This file will make sure that globally only one IF is allowed to do OTA.
 *
 * Here is the interface it should provide.
 * struct native_ota_t {
 *     QueueHandle_t dataqueue; // Queue of ota packet. will be initialized in
 * init_native_ota bool (*init_native_ota)(size_t total_size); // Start
 * native_ota. init queue if not already initialized
 *
 * }
 */

#define NATIVE_OTA_STACK 4 * 1024 // 10KB

/**
 * @brief Ota packet that is sent to the queue for writing to flash
 *
 */
typedef struct {
  unsigned char *packet;
  size_t packet_length;
  bool last_packet;
} ota_packet_t;

/**
 * @brief Enum defining tyoe of checksum that the caller provides to validate
 * the ota image
 *
 */
typedef enum {
  NATIVE_OTA_CHECKSUM_NONE,
  NATIVE_OTA_CHECKSUM_SHA256,
  NATIVE_OTA_CHECKSUM_MD5
} native_ota_checksum_type_t;

/**
 * @brief Reason that is published on events of native_ota
 *
 */
typedef enum native_ota_failure_reason_t {
  NATIVE_OTA_FAILURE_NONE = -1,
  NATIVE_OTA_FAILED_CHECKSUM_MISMATCH,
  NATIVE_OTA_INVALID_PACKET,
  NATIVE_OTA_CANCELLED,
  NATIVE_OTA_INVALID_IMAGE,
  NATIVE_OTA_UNKNOWN_ERROR,
  NATIVE_OTA_SET_PARTITION_FAIL,
} native_ota_failure_reason_t;

/**
 * @brief State in event which determines
 *
 */
typedef enum {
  NATIVE_OTA_SUCCESFUL_EVENT,
  NATIVE_OTA_FAILED_EVENT,
  NATIVE_OTA_PROGRESS_UPDATE_EVENT
} native_ota_state_t;

typedef struct native_ota_event_message_t {
  native_ota_state_t state;
  union {
    native_ota_failure_reason_t reason;
    float progress;
  };
} native_ota_event_message_t;

union native_ota_checksum_t {
  char md5_checksum[16];
  char sha256_checksum[32];
};

typedef struct {
  QueueHandle_t
      *dataqueue; // Queue of ota packet. will be initialized in init_native_ota
  esp_err_t (*init_native_ota)(
      size_t total_size, native_ota_checksum_type_t checksum_type,
      unsigned char
          *checksum); // Start native_ota. init queue if not already initialized
  void (*abort_native_ota)(void); // Abort native ota.
} native_ota_t;

/**
 * DONT EXTERN IT IN YOUR FILE. just include this header
 */
extern native_ota_t native_ota;