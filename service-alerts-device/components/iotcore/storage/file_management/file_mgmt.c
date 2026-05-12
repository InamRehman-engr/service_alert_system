
#include "esp_err.h"
#include "esp_flash.h"
#include "esp_idf_version.h"
#include "esp_log.h"
#include "esp_system.h"
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#if ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(5, 0, 0)
#include "esp_chip_info.h"
#include "spi_flash_mmap.h"
#endif

#include "file_mgmt.h"

static const char *TAG = "esp_littlefs";

esp_err_t create_directory_if_missing(const char *dir_path) {
  struct stat st;
  if (stat(dir_path, &st) == 0 && S_ISDIR(st.st_mode)) {
    ESP_LOGD(TAG, "Directory already exists: %s", dir_path);
    return ESP_OK;
  }
  if (mkdir(dir_path, 0777) == 0 || errno == EEXIST) {
    ESP_LOGD(TAG, "Directory created or already exists: %s", dir_path);
    return ESP_OK;
  } else {
    ESP_LOGE(TAG, "Failed to create directory: %s, errno: %d", dir_path, errno);
    return ESP_FAIL;
  }
}

esp_err_t create_file_in_littlefs(const char *path) {
  char dir_path[256];
  strncpy(dir_path, path, sizeof(dir_path) - 1);
  dir_path[sizeof(dir_path) - 1] = '\0';

  char *last_slash = strrchr(dir_path, '/');
  if (last_slash) {
    *last_slash = '\0'; // Terminate at the last '/'
    if (create_directory_if_missing(dir_path) != ESP_OK) {
      return ESP_FAIL;
    }
  }

  FILE *file = fopen(path, "wb");
  if (!file) {
    ESP_LOGE(TAG, "Failed to create file: %s", path);
    return ESP_FAIL;
  }
  fclose(file);
  ESP_LOGD(TAG, "File created successfully: %s", path);
  return ESP_OK;
}

bool file_exists_in_littlefs(const char *path) {
  FILE *file = fopen(path, "r");
  if (file) {
    fclose(file);
    return true; // File exists
  }
  return false; // File does not exist
}

esp_err_t get_file_size(const char *path, size_t *file_size) {
  FILE *file = fopen(path, "rb"); // Open file in binary mode
  if (file == NULL) {
    ESP_LOGE(TAG, "Failed to open file: %s", path);
    return ESP_FAIL;
  }
  if (fseek(file, 0,
            SEEK_END)) { // Move the file pointer to the end of the file
    ESP_LOGE(TAG, "Failed to seek END");
    fclose(file);
    return ESP_FAIL;
  }
  *file_size = ftell(file);
  fclose(file);
  return ESP_OK;
}

esp_err_t delete_file_from_littlefs(const char *path) {
  // Try to delete the file at the given path
  if (remove(path) == 0) {
    ESP_LOGI(TAG, "File deleted successfully: %s", path);
    return ESP_OK;
  } else {
    ESP_LOGE(TAG, "Failed to delete file: %s", path);
    return ESP_ERR_NOT_FOUND; // Return an error if the file couldn't be deleted
  }
}

esp_err_t write_file_data(const char *path, const char *data,
                          size_t data_size) {
  return write_file_data_at_offset(path, data, data_size, 0);
}

esp_err_t append_data_to_file(const char *path, const char *data,
                              size_t data_size) {
  // Open the file in read/write mode
  FILE *file = fopen(path, "r+b");
  if (!file) {
    // If the file doesn't exist, create it
    file = fopen(path, "w+b");
    if (!file) {
      ESP_LOGE(TAG, "Failed to open or create file: %s", path);
      return ESP_ERR_NOT_FOUND;
    }
  }
  if (fseek(file, 0, SEEK_END) != 0) {
    ESP_LOGE(TAG, "Failed to seek to the end of the file: %s", path);
    fclose(file);
    return ESP_ERR_INVALID_STATE;
  }
  // Write data to the file
  size_t written = fwrite(data, 1, data_size, file);
  if (written != data_size) {
    ESP_LOGE(TAG, "Failed to write all data to file: %s", path);
    fclose(file);
    return ESP_ERR_INVALID_STATE;
  }
  fclose(file);
  ESP_LOGD(TAG, "Data appended successfully to file: %s", path);
  return ESP_OK;
}

esp_err_t write_file_data_at_offset(const char *path, const char *data,
                                    size_t data_size, uint32_t offset) {
  // Open the file in read/write mode
  FILE *file = fopen(path, "r+b");
  if (!file) {
    // If the file doesn't exist, create it
    file = fopen(path, "w+b");
    if (!file) {
      ESP_LOGE(TAG, "Failed to open or create file: %s", path);
      return ESP_ERR_NOT_FOUND;
    }
  }
  if (fseek(file, offset, SEEK_SET) != 0) {
    ESP_LOGE(TAG, "Failed to seek to offset: %" PRIu32, offset);
    fclose(file);
    return ESP_ERR_INVALID_STATE;
  }
  size_t written = fwrite(data, 1, data_size, file);
  if (written != data_size) {
    ESP_LOGE(TAG, "Failed to write all data to file: %s", path);
    fclose(file);
    return ESP_ERR_INVALID_STATE;
  }
  fclose(file);
  ESP_LOGD(TAG, "File written successfully: %s", path);
  return ESP_OK;
}

esp_err_t read_file_data(const char *path, unsigned char *buffer,
                         size_t *read_data_length) {
  return read_file_data_at_offset(path, buffer, read_data_length, 0);
}

esp_err_t read_file_data_at_offset(const char *path, unsigned char *buffer,
                                   size_t *read_data_length, uint32_t offset) {
  FILE *file = fopen(path, "rb"); // Open file in binary mode
  if (file == NULL) {
    ESP_LOGE(TAG, "Failed to open file for reading: %s", path);
    return ESP_FAIL;
  }
  if (fseek(file, offset, SEEK_SET) != 0) {
    ESP_LOGE(TAG, "Failed to seek to offset: %" PRIu32, offset);
    fclose(file);
    return ESP_FAIL;
  }
  size_t bytes_read =
      fread(buffer, sizeof(unsigned char), *read_data_length, file);
  if (bytes_read < *read_data_length) {
    if (feof(file)) {
      ESP_LOGD(TAG, "Reached end of file. Bytes read: %zu", bytes_read);
    } else {
      ESP_LOGE(TAG, "Error reading file data");
      fclose(file);
      return ESP_FAIL;
    }
  } else {
    ESP_LOGD(TAG, "Bytes read: %zu", bytes_read);
  }
  *read_data_length = bytes_read;
  fclose(file);
  return ESP_OK;
}

void init_littlefs(esp_vfs_littlefs_conf_t *conf) {
  // Use settings defined above to initialize and mount LittleFS filesystem.
  // Note: esp_vfs_littlefs_register is an all-in-one convenience function.
  esp_err_t ret = esp_vfs_littlefs_register(conf);

  if (ret != ESP_OK) {
    if (ret == ESP_FAIL) {
      ESP_LOGE(TAG, "Failed to mount or format filesystem");
    } else if (ret == ESP_ERR_NOT_FOUND) {
      ESP_LOGE(TAG, "Failed to find LittleFS partition");
    } else {
      ESP_LOGE(TAG, "Failed to initialize LittleFS (%s)", esp_err_to_name(ret));
    }
    return;
  }

  size_t total = 0, used = 0;
  ret = esp_littlefs_info(conf->partition_label, &total, &used);
  if (ret != ESP_OK) {
    ESP_LOGE(TAG, "Failed to get LittleFS partition information (%s)",
             esp_err_to_name(ret));
  } else {
    ESP_LOGI(TAG, "Partition size: total: %d, used: %d", total, used);
  }
}

void deinit_littlefs(esp_vfs_littlefs_conf_t *conf) {
  // All done, unmount partition and disable LittleFS
  esp_err_t ret = esp_vfs_littlefs_unregister(conf->partition_label);
  switch (ret) {
  case ESP_OK:
    ESP_LOGI(TAG, "LittleFS unmounted");
    break;
  case ESP_ERR_INVALID_STATE:
    ESP_LOGW(TAG, "LittleFS is already unmounted");
    break;
  default:
    ESP_LOGE(TAG, "LittleFS failed to deinitialize (%s)", esp_err_to_name(ret));
    break;
  }
}