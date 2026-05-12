#include "sysinfo.h"
#include "string.h"

// DONT EXTERN IN YOUR FILES. Not cool
char *registrationNumber = NULL;
char *devicePin = NULL;
char *hardwareVersion = NULL;
char *softwareVersion = NULL;

device_info_t deviceInfo;

char *getDevicePin() {
  if (devicePin == NULL) {
    uint8_t mac_default[6];
    esp_efuse_mac_get_default((uint8_t *)mac_default);
    int pin = mac_default[0] + mac_default[1] + mac_default[2] +
              mac_default[3] + mac_default[4] + mac_default[5];
    asprintf(&devicePin, "%04d", (pin & 0xFFFF));
  }
  return devicePin;
}
char *getRegistrationNumber() {
  if (registrationNumber == NULL) {
    uint8_t mac_default[6];
    esp_efuse_mac_get_default((uint8_t *)mac_default);
    esp_chip_info_t chip_info;
    esp_chip_info(&chip_info);
    asprintf(&registrationNumber, "%02X%02X%02X%02X%02X%02XC%dR%d",
             mac_default[0], mac_default[1], mac_default[2], mac_default[3],
             mac_default[4], mac_default[5], chip_info.cores,
             chip_info.revision);
  }
  return registrationNumber;
}

/// TODO: Add rc handling
char *getFirmwareVersion() {
  if (softwareVersion == NULL) {

#ifdef CONFIG_FW_IS_RC
    asprintf(&softwareVersion, "v%d.%d.%d-rc%d",
              CONFIG_CURRENT_FIRMWARE_MAJOR_VERSION,
              CONFIG_CURRENT_FIRMWARE_MINOR_VERSION,
              CONFIG_CURRENT_FIRMWARE_SUB_VERSION, CONFIG_FW_RC_NUM);
#else
    asprintf(&softwareVersion, "v%d.%d.%d",
              CONFIG_CURRENT_FIRMWARE_MAJOR_VERSION,
              CONFIG_CURRENT_FIRMWARE_MINOR_VERSION,
              CONFIG_CURRENT_FIRMWARE_SUB_VERSION);
#endif
  }
  return softwareVersion;
}

char *getHardwareVersion() {
  if (hardwareVersion == NULL) {
    asprintf(&hardwareVersion, "%s_%s", CONFIG_DEVICE_HARDWARE_PREFIX,
              CONFIG_HARDWARE_VERSION);
  }
  return hardwareVersion;
}

device_info_t *getDeviceInfo() {
  deviceInfo.devicePin = getDevicePin();
  deviceInfo.registrationNumber = getRegistrationNumber();
  deviceInfo.hardwareVersion = getHardwareVersion();
  deviceInfo.firmwareVersion = getFirmwareVersion();
  deviceInfo.deviceTypeId = CONFIG_DEVICE_TYPE_ID;
  return &deviceInfo;
}
