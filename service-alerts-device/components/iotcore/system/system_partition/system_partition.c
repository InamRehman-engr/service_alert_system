#include "system_partition.h"
#include "esp_log.h"
#include "esp_ota_ops.h"
#include "esp_partition.h"
#include "esp_system.h"
#include "string.h"

#define TAG "system_partition"

esp_err_t setBootPartition(const char *partition_label) {
  esp_err_t err = ESP_FAIL;
  const esp_partition_t *current_running_partition =
      esp_ota_get_running_partition();

  ESP_LOGD(TAG, "current running partition: %s",
           current_running_partition->label);

  esp_partition_iterator_t iterator = esp_partition_find(
      ESP_PARTITION_TYPE_ANY, ESP_PARTITION_SUBTYPE_ANY, partition_label);

  if (iterator == NULL) {
    ESP_LOGE(TAG, "partition not found");
    return err;
  }
  const esp_partition_t *target_partition = esp_partition_get(iterator);
  if (target_partition == NULL) {
    ESP_LOGE(TAG, "partition not found");
    return err;
  }

  if (target_partition == current_running_partition) {
    ESP_LOGW(TAG, "partition is already running");
    return err;
  }
  err = esp_ota_set_boot_partition(target_partition);
  if (err != ESP_OK) {
    ESP_LOGE(TAG, "esp_ota_set_boot_partition failed");
    return err;
  }

  ESP_LOGI(TAG, "esp_ota_set_boot_partition success");

  esp_partition_iterator_release(iterator);

  ESP_LOGI(TAG, "restarting");
  esp_restart();
}
const char *get_running_partition_label() {
  const esp_partition_t *current_running_partition =
      esp_ota_get_running_partition();
  return current_running_partition->label;
}