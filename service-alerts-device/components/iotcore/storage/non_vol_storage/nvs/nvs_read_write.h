#ifndef _nvs_read_write_h_
#define _nvs_read_write_h_

#include "esp_err.h"
#include "esp_log.h"
esp_err_t nvs_getlock(void);
esp_err_t nvs_releaselock(void);

esp_err_t readKeyValueInFlash_blob(char *key, uint8_t *data, size_t *len);
esp_err_t readKeyValueInFlash_str(char *key, char *data, size_t *size);
esp_err_t saveKeyValueInFlash(char *key, uint8_t *data, int32_t size);
esp_err_t saveKeyValueInFlash_str(char *key, char *data);
#define saveKeyValueInFlash_blob saveKeyValueInFlash
esp_err_t saveKeyValueInFlash_int32(char *key, int32_t data);
esp_err_t readKeyValueInFlash_int32(char *key, int32_t *data);
esp_err_t nvs_read_write_init(char *name);
esp_err_t erase_key_from_flash(char *key);
esp_err_t saveKeyValueInFlash_int64(char *key, int64_t data);
esp_err_t readKeyValueInFlash_int64(char *key, int64_t *data);

// esp_err_t saveKeyValueInFlash_blob_ns(char *key, uint8_t *data, char
// *namespace,
//                                       int32_t size);
// esp_err_t readKeyValueInFlash_blob_ns(char *key, uint8_t *data, char
// *namespace,
//                                       size_t *len);
// esp_err_t deleteKeyValueInFlash_blob_ns(char *key, char *namespace);
// esp_err_t erase_namespace(char *namespace);
#endif