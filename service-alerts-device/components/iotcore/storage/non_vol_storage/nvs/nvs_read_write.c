#include "nvs_read_write.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "nvs.h"
#include "nvs_flash.h"

#define TAG "NVS"

SemaphoreHandle_t xSemaphore_nvs = NULL; /// Semaphore for nvs

esp_err_t nvs_getlock(void) {
  if (xSemaphoreTake(xSemaphore_nvs, 4000) == pdFALSE) {
    ESP_LOGE(TAG, "unable to get lock %d ", __LINE__);
    return ESP_FAIL;
  }
  return ESP_OK;
}
esp_err_t nvs_releaselock(void) {
  xSemaphoreGive(xSemaphore_nvs);
  return ESP_OK;
}
esp_err_t readKeyValueInFlash_blob(char *key, uint8_t *data, size_t *len) {
  if (xSemaphoreTake(xSemaphore_nvs, 4000) == pdFALSE) {
    ESP_LOGE(TAG, "unable to get lock %d ", __LINE__);
    return ESP_FAIL;
  }
  nvs_handle my_handle;
  esp_err_t err = nvs_open("storage", NVS_READWRITE, &my_handle);

  err = nvs_get_blob(my_handle, key, data, len);
  switch (err) {
  case ESP_OK:
    break;
  case ESP_ERR_NVS_NOT_FOUND:
    ESP_LOGD(TAG, "[%s] The value is not initialized yet!", key);
    break;
  default:
    ESP_LOGE(TAG, "[%s] Error (%s) reading!", key, esp_err_to_name(err));
  }
  // Close
  nvs_close(my_handle);
  xSemaphoreGive(xSemaphore_nvs);
  return err;
}

esp_err_t readKeyValueInFlash_str(char *key, char *data, size_t *size) {
  // Read
  esp_err_t err = ESP_FAIL;
  if (xSemaphoreTake(xSemaphore_nvs, 4000) == pdTRUE) {
    nvs_handle my_handle;
    err = nvs_open("storage", NVS_READWRITE, &my_handle);
    err = nvs_get_str(my_handle, key, (char *)data, size);
    switch (err) {
    case ESP_OK:
      break;
    case ESP_ERR_NVS_NOT_FOUND:
      ESP_LOGD(TAG, "[%s] The value is not initialized yet!", key);
      break;
    default:
      ESP_LOGE(TAG, "[%s] Error (%s) reading!", key, esp_err_to_name(err));
    }
    // Close
    nvs_close(my_handle);
    xSemaphoreGive(xSemaphore_nvs);
  } else {
    ESP_LOGE(TAG, "unable to get lock %d ", __LINE__);
  }
  return err;
}
esp_err_t saveKeyValueInFlash(char *key, uint8_t *data, int32_t size) {
  if (xSemaphoreTake(xSemaphore_nvs, 4000) == pdFALSE) {
    ESP_LOGE(TAG, "unable to get lock %d ", __LINE__);
    return ESP_FAIL;
  }
  nvs_handle my_handle;
  esp_err_t err = nvs_open("storage", NVS_READWRITE, &my_handle);
  if (err == ESP_OK) {
    if (size == 0)
      err = nvs_set_str(my_handle, key, (char *)data);
    else
      err = nvs_set_blob(my_handle, key, data, size);
  }
  if (err == ESP_OK)
    err = nvs_commit(my_handle);
  if (err != ESP_OK) {
    ESP_LOGE(TAG, "Error (%s) writing %s !", key, esp_err_to_name(err));
  }
  // Close
  nvs_close(my_handle);
  xSemaphoreGive(xSemaphore_nvs);
  return err;
}
esp_err_t saveKeyValueInFlash_str(char *key, char *data) {
  if (xSemaphoreTake(xSemaphore_nvs, 4000) == pdFALSE) {
    ESP_LOGE(TAG, "unable to get lock %d ", __LINE__);
    return ESP_FAIL;
  }
  nvs_handle my_handle;
  esp_err_t err = nvs_open("storage", NVS_READWRITE, &my_handle);
  if (err == ESP_OK) {
    err = nvs_set_str(my_handle, key, (char *)data);
  }
  if (err == ESP_OK)
    err = nvs_commit(my_handle);
  if (err != ESP_OK) {
    ESP_LOGE(TAG, "Error (%s) writing %s !", key, esp_err_to_name(err));
  }
  // Close
  nvs_close(my_handle);
  xSemaphoreGive(xSemaphore_nvs);
  return err;
}
esp_err_t saveKeyValueInFlash_int32(char *key, int32_t data) {
  if (xSemaphoreTake(xSemaphore_nvs, 4000) == pdFALSE) {
    ESP_LOGE(TAG, "unable to get lock %d ", __LINE__);
    return ESP_FAIL;
  }
  nvs_handle my_handle;
  esp_err_t err = nvs_open("storage", NVS_READWRITE, &my_handle);
  if (err == ESP_OK) {
    err = nvs_set_i32(my_handle, key, data);
  }
  if (err == ESP_OK)
    err = nvs_commit(my_handle);
  if (err != ESP_OK) {
    ESP_LOGE(TAG, "Error (%s) writing %s !", key, esp_err_to_name(err));
  }
  // Close
  nvs_close(my_handle);
  xSemaphoreGive(xSemaphore_nvs);
  return err;
}

esp_err_t readKeyValueInFlash_int64(char *key, int64_t *data) {
  if (xSemaphoreTake(xSemaphore_nvs, 4000) == pdFALSE) {
    ESP_LOGE(TAG, "unable to get lock %d ", __LINE__);
    return ESP_FAIL;
  }
  nvs_handle my_handle;
  esp_err_t err = nvs_open("storage", NVS_READWRITE, &my_handle);
  err = nvs_get_i64(my_handle, key, data);
  switch (err) {
  case ESP_OK:
    break;
  case ESP_ERR_NVS_NOT_FOUND:
    ESP_LOGD(TAG, "[%s] The value is not initialized yet!", key);
    break;
  default:
    ESP_LOGE(TAG, "[%s] Error (%s) reading!", key, esp_err_to_name(err));
  }
  // Close
  nvs_close(my_handle);
  xSemaphoreGive(xSemaphore_nvs);
  return err;
}

esp_err_t saveKeyValueInFlash_int64(char *key, int64_t data) {
  if (xSemaphoreTake(xSemaphore_nvs, 4000) == pdFALSE) {
    ESP_LOGE(TAG, "unable to get lock %d ", __LINE__);
    return ESP_FAIL;
  }
  nvs_handle my_handle;
  esp_err_t err = nvs_open("storage", NVS_READWRITE, &my_handle);
  if (err == ESP_OK) {
    err = nvs_set_i64(my_handle, key, data);
  }
  if (err == ESP_OK)
    err = nvs_commit(my_handle);
  if (err != ESP_OK) {
    ESP_LOGE(TAG, "Error (%s) writing %s !", key, esp_err_to_name(err));
  }
  // Close
  nvs_close(my_handle);
  xSemaphoreGive(xSemaphore_nvs);
  return err;
}

esp_err_t readKeyValueInFlash_int32(char *key, int32_t *data) {
  if (xSemaphoreTake(xSemaphore_nvs, 4000) == pdFALSE) {
    ESP_LOGE(TAG, "unable to get lock %d ", __LINE__);
    return ESP_FAIL;
  }
  // Read
  nvs_handle my_handle;
  esp_err_t err = nvs_open("storage", NVS_READWRITE, &my_handle);
  err = nvs_get_i32(my_handle, key, data);
  switch (err) {
  case ESP_OK:
    break;
  case ESP_ERR_NVS_NOT_FOUND:
    ESP_LOGD(TAG, "[%s] The value is not initialized yet!", key);
    break;
  default:
    ESP_LOGE(TAG, "[%s] Error (%s) reading!", key, esp_err_to_name(err));
  }
  // Close
  nvs_close(my_handle);
  xSemaphoreGive(xSemaphore_nvs);
  return err;
}

esp_err_t erase_key_from_flash(char *key) {
  if (xSemaphoreTake(xSemaphore_nvs, 4000) == pdFALSE) {
    ESP_LOGE(TAG, "unable to get lock %d ", __LINE__);
    return ESP_FAIL;
  }
  // Read
  nvs_handle my_handle;
  esp_err_t err = nvs_open("storage", NVS_READWRITE, &my_handle);
  err = nvs_erase_key(my_handle, key);
  switch (err) {
  case ESP_OK:
    break;
  case ESP_ERR_NVS_NOT_FOUND:
    ESP_LOGD(TAG, "[%s] The value is not initialized yet!", key);
    break;
  default:
    ESP_LOGE(TAG, "[%s] Error (%s) reading!", key, esp_err_to_name(err));
  }
  // Close
  nvs_close(my_handle);
  xSemaphoreGive(xSemaphore_nvs);
  return err;
}
esp_err_t nvs_read_write_init(char *name) {
  esp_err_t err = nvs_flash_init();
  if (err == ESP_ERR_NVS_NO_FREE_PAGES ||
      err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
    // NVS partition was truncated and needs to be erased
    // Retry nvs_flash_init
    ESP_ERROR_CHECK(nvs_flash_erase());
    err = nvs_flash_init();
  }
  xSemaphore_nvs = xSemaphoreCreateMutex();
  return err;
}
esp_err_t saveKeyValueInFlash_blob_ns(char *key, uint8_t *data, char *namespace,
                                      int32_t size) {
  if (xSemaphoreTake(xSemaphore_nvs, 4000) == pdFALSE) {
    ESP_LOGE(TAG, "unable to get lock %d ", __LINE__);
    return ESP_FAIL;
  }
  nvs_handle my_handle;
  esp_err_t err = nvs_open(namespace, NVS_READWRITE, &my_handle);
  if (err == ESP_OK) {
    if (size == 0)
      err = nvs_set_str(my_handle, key, (char *)data);
    else
      err = nvs_set_blob(my_handle, key, data, size);
  }
  if (err == ESP_OK)
    err = nvs_commit(my_handle);
  if (err != ESP_OK) {
    ESP_LOGE(TAG, "Error (%s) writing %s !", key, esp_err_to_name(err));
  }
  // Close
  nvs_close(my_handle);
  xSemaphoreGive(xSemaphore_nvs);
  return err;
}

esp_err_t readKeyValueInFlash_blob_ns(char *key, uint8_t *data, char *namespace,
                                      size_t *len) {
  if (xSemaphoreTake(xSemaphore_nvs, 4000) == pdFALSE) {
    ESP_LOGE(TAG, "unable to get lock %d ", __LINE__);
    return ESP_FAIL;
  }
  // Read
  nvs_handle my_handle;
  esp_err_t err = nvs_open(namespace, NVS_READWRITE, &my_handle);

  err = nvs_get_blob(my_handle, key, data, len);
  switch (err) {
  case ESP_OK:
    break;
  case ESP_ERR_NVS_NOT_FOUND:
    ESP_LOGD(TAG, "[%s] The value is not initialized yet!", key);
    break;
  default:
    ESP_LOGE(TAG, "[%s] Error (%s) reading!", key, esp_err_to_name(err));
  }
  // Close
  nvs_close(my_handle);
  xSemaphoreGive(xSemaphore_nvs);
  return err;
}

esp_err_t deleteKeyValueInFlash_blob_ns(char *key, char *namespace) {
  if (xSemaphoreTake(xSemaphore_nvs, 4000) == pdFALSE) {
    ESP_LOGE(TAG, "unable to get lock %d ", __LINE__);
    return ESP_FAIL;
  }
  // Read
  nvs_handle my_handle;
  esp_err_t err = nvs_open(namespace, NVS_READWRITE, &my_handle);
  err = nvs_erase_key(my_handle, key);
  switch (err) {
  case ESP_OK:
    nvs_commit(my_handle);
    break;
  case ESP_ERR_NVS_NOT_FOUND:
    ESP_LOGD(TAG, "[%s] The value is not initialized yet!", key);
    break;
  default:
    ESP_LOGE(TAG, "[%s] Error (%s) reading!", key, esp_err_to_name(err));
  }
  // Close
  nvs_close(my_handle);
  xSemaphoreGive(xSemaphore_nvs);
  return err;
}
esp_err_t erase_namespace(char *namespace) {
  if (xSemaphoreTake(xSemaphore_nvs, 4000) == pdFALSE) {
    ESP_LOGE(TAG, "unable to get lock %d ", __LINE__);
    return ESP_FAIL;
  }
  nvs_handle my_handle;
  esp_err_t err = nvs_open(namespace, NVS_READWRITE, &my_handle);
  err = nvs_erase_all(my_handle);
  switch (err) {
  case ESP_OK:
    nvs_commit(my_handle);
    break;
  case ESP_ERR_NVS_NOT_FOUND:
    ESP_LOGD(TAG, "[%s] The value is not initialized yet!", namespace);
    break;
  default:
    ESP_LOGE(TAG, "[%s] Error (%s) reading!", namespace, esp_err_to_name(err));
  }
  nvs_close(my_handle);
  xSemaphoreGive(xSemaphore_nvs);
  return err;
}
