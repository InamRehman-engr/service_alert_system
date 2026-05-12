#include "iot_core_spiffs.h"

// NEED FIX
uint8_t spiffs_init(const char *partition_label, size_t max_files,
                    bool format_fs) {
  ESP_LOGI(TAG, "Initializing SPIFFS");
  esp_vfs_spiffs_conf_t conf = {.base_path = "/spiffs",
                                .partition_label = partition_label,
                                .max_files = max_files,
                                .format_if_mount_failed = true};

  esp_err_t spiffs_register_return_code =
      esp_vfs_spiffs_register(&conf); // Register and mount SPIFFS to VFS

  if (spiffs_register_return_code != ESP_OK) {
    if (spiffs_register_return_code == ESP_FAIL) {
      ESP_LOGE(TAG, "Failed to mount or format filesystem");
      return spiffs_register_return_code;
    } else if (spiffs_register_return_code == ESP_ERR_NOT_FOUND) {
      ESP_LOGE(TAG, "Failed to find SPIFFS partition");
      return spiffs_register_return_code;
    } else if (spiffs_register_return_code == ESP_ERR_INVALID_STATE) {
      ESP_LOGE(TAG,
               "Failed! Filesystem is already mounted or partition is "
               "encrypted (%s)",
               esp_err_to_name(spiffs_register_return_code));
      return spiffs_register_return_code;
    } else if (spiffs_register_return_code == ESP_ERR_NO_MEM) {
      ESP_LOGE(TAG, "Failed to allocate. Insufficient memory (%s)",
               esp_err_to_name(spiffs_register_return_code));
      return spiffs_register_return_code;
    }
  } else {
    ESP_LOGI(TAG, "SPIFFS mounted!");
  }

  // // Format
  // if (format_fs)
  // {
  //     esp_err_t esp_spiffs_format(NULL);
  // }
  return 0;
}