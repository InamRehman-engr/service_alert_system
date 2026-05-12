#pragma once
#include "esp_log.h"
#include "esp_ota_ops.h"
#include "sdkconfig.h"
#include "string.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

#define STR_HELPER(x) #x
#define STR(x) STR_HELPER(x)

enum firmwareInfo {
  FIRMWARE_VERSION_OK = 0,
  FIRMWARE_VERSION_MISMATCH,
  INVALID_CURRENT_FIRMWARE_STRING,
  INVALID_INCOMING_FIRMWARE_STRING,
  INVALID_CURRENT_RC_FIRMWARE_STRING,
  INVALID_INCOMING_RC_FIRMWARE_STRING,
  FIRMWARE_VERSION_SAME,
  FIRMWARE_DOWNGRADE_FAIL,
};

enum hardwareInfo {
  HARDWARE_VERSION_OK = 0,
  HARDWARE_VERSION_MISMATCH,
  INVALID_INCOMING_HARDWARE_STRING,
  INVALID_CURRENT_HARDWARE_STRING,
};

enum firmwareInfo validate_firmware_version(char *incomingFirmwareVer,
                                            char *currentFirmwareVer,
                                            bool downgrade);
enum hardwareInfo validate_hardware_version(char *incomingHardwareVer,
                                            char *currentHardwareVer);

void rollback_firmware_checker(void);