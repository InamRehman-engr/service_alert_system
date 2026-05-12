#pragma once

#include "sdkconfig.h"
#include "string.h"
#include <stdbool.h>
#include <stdint.h>

#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"

#include "esp_system.h"

#define DELTA_OTA_STACK 4 * 1024 // 4KB

typedef struct {
  unsigned char *packet;
  size_t packet_length;
  bool last_packet;
} delta_ota_packet_t;

typedef enum {
  DELTA_OTA_CHECKSUM_NONE,
  DELTA_OTA_CHECKSUM_SHA256,
  DELTA_OTA_CHECKSUM_MD5
} delta_ota_checksum_type_t;

typedef enum delta_ota_failure_reason_t {
  DELTA_OTA_FAILURE_NONE = -1,
  DELTA_OTA_FAILED_CHECKSUM_MISMATCH,
  DELTA_OTA_INVALID_PACKET,
  DELTA_OTA_CANCELLED,
  DELTA_OTA_INVALID_IMAGE,
  DELTA_OTA_UNKNOWN_ERROR,
  DELTA_OTA_SET_PARTITION_FAIL,
  DELTA_OTA_FAILED_FINALIZE,
} delta_ota_failure_reason_t;

typedef enum delta_ota_image_failure_reason_t {
  DELTA_OTA_IMAGE_VALID = -1,
  DELTA_OTA_IMAGE_NULL_HEADER,
  DELTA_OTA_IMAGE_INVALID_MAGIC_WORD,
  DELTA_OTA_IMAGE_INVALID_CHECKSUM,
} delta_ota_image_failure_reason_t;

typedef enum {
  DELTA_OTA_SUCCESFUL_EVENT,
  DELTA_OTA_FAILED_EVENT,
  DELTA_OTA_PROGRESS_UPDATE_EVENT,
} delta_ota_state_t;

typedef struct delta_ota_event_message_t {
  delta_ota_state_t state;
  union {
    delta_ota_failure_reason_t reason;
    float progress;
  };
} delta_ota_event_message_t;

union delta_ota_checksum_t {
  char md5_checksum[16];
  char sha256_checksum[32];
};

typedef struct {
  QueueHandle_t *dataqueue;
  esp_err_t (*init_delta_ota)(size_t total_size,
                              delta_ota_checksum_type_t checksum_type,
                              unsigned char *checksum);
  void (*abort_delta_ota)(void);
} delta_ota_t;

/**
 * DONT EXTERN IT IN YOUR FILE. just include this header
 */
extern delta_ota_t delta_ota;
extern bool delta_ota_image_validated;