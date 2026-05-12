/* test_mean.c: Implementation of a testable component.

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/

#include "esp_err.h"
#include "nvs_read_write.h"
#include "unity.h"
#include <esp_ota_ops.h>
#include <limits.h>

#define countof(x) (sizeof(x) / sizeof(x[0]))

TEST_CASE("storage is working", "[storage]") {
  int values = 0;
  TEST_ASSERT_EQUAL(ESP_OK, nvs_read_write_init(NULL));
  TEST_ASSERT_EQUAL(ESP_OK, saveKeyValueInFlash_int32("unittest", 10));
  TEST_ASSERT_EQUAL(ESP_OK, readKeyValueInFlash_int32("unittest", &values));
  TEST_ASSERT_EQUAL(values, 10);
}

TEST_CASE("esp_ota_get_next_update_partition logic", "[storage]") {
  const esp_partition_t *running = esp_ota_get_running_partition();
  const esp_partition_t *factory = esp_partition_find_first(
      ESP_PARTITION_TYPE_APP, ESP_PARTITION_SUBTYPE_APP_FACTORY, NULL);
  const esp_partition_t *ota_0 = esp_partition_find_first(
      ESP_PARTITION_TYPE_APP, ESP_PARTITION_SUBTYPE_APP_OTA_0, NULL);
  const esp_partition_t *ota_1 = esp_partition_find_first(
      ESP_PARTITION_TYPE_APP, ESP_PARTITION_SUBTYPE_APP_OTA_1, NULL);
  const esp_partition_t *ota_2 = esp_partition_find_first(
      ESP_PARTITION_TYPE_APP, ESP_PARTITION_SUBTYPE_APP_OTA_2, NULL);

  TEST_ASSERT_NOT_NULL(running);
  TEST_ASSERT_NULL(factory);
  TEST_ASSERT_NOT_NULL(ota_0);
  TEST_ASSERT_NOT_NULL(ota_1);
  TEST_ASSERT_NULL(
      ota_2); /* this partition shouldn't exist in test partition table */

  TEST_ASSERT_NOT_EQUAL(factory,
                        running); /* this may not be true if/when we get OTA
                                     tests that do OTA updates */

  const esp_partition_t *p = NULL;
  /* OTA slot 0 updates OTA slot 1 */
  p = esp_ota_get_next_update_partition(ota_0);
  TEST_ASSERT_EQUAL_HEX8(ESP_PARTITION_SUBTYPE_APP_OTA_1, p->subtype);
  TEST_ASSERT_EQUAL_PTR(ota_1, p);
  /* OTA slot 1 updates OTA slot 0 */
  p = esp_ota_get_next_update_partition(ota_1);
  TEST_ASSERT_EQUAL_HEX8(ESP_PARTITION_SUBTYPE_APP_OTA_0, p->subtype);
  ;
  TEST_ASSERT_EQUAL_PTR(ota_0, p);
}