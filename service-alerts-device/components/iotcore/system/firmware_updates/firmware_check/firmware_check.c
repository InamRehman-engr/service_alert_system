#include "firmware_check.h"

static const char *TAG = "firmare_check";

enum firmwareInfo validate_firmware_version(char *incomingFirmwareVer,
                                            char *currentFirmwareVer,
                                            bool downgrade) {
  ESP_LOGI(TAG, "Validating firmware informwation");
  int firmware_major = 0, firmware_minor = 0, firmware_sub = 0, firmware_rc = 0,
      chars_parsed = 0, firmware_match = 0;
  firmware_match = sscanf(incomingFirmwareVer, "v%d.%d.%d%n", &firmware_major,
                          &firmware_minor, &firmware_sub, &chars_parsed);
  if (firmware_match != 3 && incomingFirmwareVer[chars_parsed] != '-' &&
      incomingFirmwareVer[chars_parsed] != '\0') {
    ESP_LOGI(TAG, "Invalid Incoming firmware string");
    return INVALID_INCOMING_FIRMWARE_STRING;
  }
  if (incomingFirmwareVer[chars_parsed] == '-') // A release candidate firmware
  {
    firmware_match =
        sscanf(incomingFirmwareVer, "v%d.%d.%d-rc%d%n", &firmware_major,
               &firmware_minor, &firmware_sub, &firmware_rc, &chars_parsed);
    if (firmware_match != 4 && incomingFirmwareVer[chars_parsed] != '\0') {
      ESP_LOGI(TAG, "Invalid incoming RC firmware string");
      return INVALID_INCOMING_RC_FIRMWARE_STRING;
    }
  }
  firmware_match = sscanf(currentFirmwareVer, "v%d.%d.%d%n", &firmware_major,
                          &firmware_minor, &firmware_sub, &chars_parsed);
  if (firmware_match != 3 && currentFirmwareVer[chars_parsed] != '-' &&
      currentFirmwareVer[chars_parsed] != '\0') {
    ESP_LOGI(TAG, "Invalid current firmware string");
    return INVALID_CURRENT_FIRMWARE_STRING;
  }
  if (currentFirmwareVer[chars_parsed] == '-') // A release candidate firmware
  {
    firmware_match =
        sscanf(currentFirmwareVer, "v%d.%d.%d-rc%d%n", &firmware_major,
               &firmware_minor, &firmware_sub, &firmware_rc, &chars_parsed);
    if (firmware_match != 4 && currentFirmwareVer[chars_parsed] != '\0') {
      ESP_LOGI(TAG, "Invalid current rc firmware string");
      return INVALID_CURRENT_RC_FIRMWARE_STRING;
    }
  }
  if (strlen(incomingFirmwareVer) > strlen(currentFirmwareVer)) {
    int firmware_version_result = strncmp(
        incomingFirmwareVer, currentFirmwareVer, strlen(currentFirmwareVer));
    if (firmware_version_result <= 0) {
      ESP_LOGI(TAG, "Downgrade Firmware %s",
               downgrade ? "successfull" : "failed");
      return downgrade ? FIRMWARE_VERSION_OK : FIRMWARE_DOWNGRADE_FAIL;
    } else {
      ESP_LOGI(TAG, "Upgrading Firmware version");
      return FIRMWARE_VERSION_OK;
    }
  } else if (strlen(incomingFirmwareVer) < strlen(currentFirmwareVer)) {
    int firmware_version_result = strncmp(
        incomingFirmwareVer, currentFirmwareVer, strlen(incomingFirmwareVer));
    if (firmware_version_result < 0) {
      ESP_LOGI(TAG, "Downgrade Firmware %s",
               downgrade ? "successfull" : "failed");
      return downgrade ? FIRMWARE_VERSION_OK : FIRMWARE_DOWNGRADE_FAIL;
    } else {
      ESP_LOGI(TAG, "Upgrading Firmware version");
      return FIRMWARE_VERSION_OK;
    }
  } else {
    int firmware_version_result = strncmp(
        incomingFirmwareVer, currentFirmwareVer, strlen(currentFirmwareVer));
    if (firmware_version_result == 0) {
      ESP_LOGI(TAG, "Same Firmware Version");
      return FIRMWARE_VERSION_SAME;
    } else if (firmware_version_result < 0) {
      ESP_LOGI(TAG, "Downgrade Firmware %s",
               downgrade ? "successfull" : "failed");
      return downgrade ? FIRMWARE_VERSION_OK : FIRMWARE_DOWNGRADE_FAIL;
    } else {
      ESP_LOGI(TAG, "Upgrading Firmware version");
      return FIRMWARE_VERSION_OK;
    }
  }
}

enum hardwareInfo validate_hardware_version(char *incomingHardwareVer,
                                            char *currentHardwareVer) {
  ESP_LOGI(TAG, "Validating hardware informwation");
  int chars_parsed = 0, hardware_major = 0, hardware_minor = 0,
      hardware_match = 0;
  char *hardware_prefix;
  hardware_match =
      sscanf(incomingHardwareVer, "%m[^_]_%d.%d%n", &hardware_prefix,
             &hardware_major, &hardware_minor, &chars_parsed);
  if (hardware_match != 3 || (incomingHardwareVer[chars_parsed] != '\0')) {
    ESP_LOGE(TAG, "Invalid Incoming hardware string");
    free(hardware_prefix); // Free dynamically allocated memory
    return INVALID_INCOMING_HARDWARE_STRING;
  }
  hardware_match =
      sscanf(currentHardwareVer, "%m[^_]_%d.%d%n", &hardware_prefix,
             &hardware_major, &hardware_minor, &chars_parsed);
  if (hardware_match != 3 || (currentHardwareVer[chars_parsed] != '\0')) {
    ESP_LOGE(TAG, "Invalid Current hardware string");
    free(hardware_prefix); // Free dynamically allocated memory
    return INVALID_CURRENT_HARDWARE_STRING;
  }
  if (strncmp(incomingHardwareVer, currentHardwareVer,
              strlen(currentHardwareVer)) != 0) {
    ESP_LOGE(TAG, "Hardware Version Mismatch");
    free(hardware_prefix); // Free dynamically allocated memory
    return HARDWARE_VERSION_MISMATCH;
  }
  ESP_LOGI(TAG, "Hardware Version is valid and matched");
  free(hardware_prefix); // Free dynamically allocated memory
  return HARDWARE_VERSION_OK;
}
void rollback_firmware_checker(void) {
#ifdef CONFIG_APP_ROLLBACK_ENABLE
  const esp_partition_t *running = esp_ota_get_running_partition();
  esp_ota_img_states_t ota_state;
  if (esp_ota_get_state_partition(running, &ota_state) == ESP_OK) {
    if (ota_state == ESP_OTA_IMG_PENDING_VERIFY) {
      if (esp_ota_mark_app_valid_cancel_rollback() == ESP_OK) {
        ESP_LOGI(TAG, "App is valid, rollback cancelled successfully");
      } else {
        ESP_LOGE(TAG, "Failed to cancel rollback");
      }
    }
  }
#else
  ESP_LOGI(TAG, "APP Rollback Enable is not set in config");
#endif
}