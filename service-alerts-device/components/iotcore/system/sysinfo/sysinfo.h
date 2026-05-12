#ifndef __sysinfo__
#define __sysinfo__

#include "esp_mac.h"
#include <esp_chip_info.h>
#include <esp_system.h>
#include <stdint.h>

/**
 * @brief Struct containing information about the system
 *
 */
typedef struct {
  char *registrationNumber; // Will contain string used for device registration.
  char *devicePin; // This will be the pin used by the device to authenticate
                   // webserver. Server will keep track of this just in case.
  char *hardwareVersion;
  char *firmwareVersion;
  uint8_t deviceTypeId;
} device_info_t;

/**
 * @brief Get the Device Info object
 *
 * @return device_info_t*
 */
device_info_t *getDeviceInfo();

#endif
