#include "system_monitoring.h"
#include "cJSON.h"
#include "esp_attr.h"
#include "esp_sleep.h"
#include "esp_timer.h"
#include "esp_types.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "iotcore_events.h"
#include "myJSON.h"
#include "nvs_read_write.h"
#include "string.h"

RTC_NOINIT_ATTR bool stackoverFlowTaskOccered = false;
// This function is already defined in esp32 but is marked with attribute weak
// meaning that it can be effectively redefined
//  Kind of like override of original. We will use it to log when a stack
//  overflow has occured
void vApplicationStackOverflowHook(struct tskTaskControlBlock *xTask,
                                   char *pcTaskName) {
  // Post of stack overflow now through theough events in the iotcore_event_loop
  esp_rom_printf("Stack Overflow Hook: A stack overflow in task \"%s\" has "
                 "been detected.\r\n",
                 pcTaskName);
  post_iotcore_error_event(TASK_STACK_OVERFLOW, (char *)pcTaskName,
                           sizeof((char *)pcTaskName));
  vTaskDelay(pdMS_TO_TICKS(1000)); // Give time for event post
  abort();
  // Since this is stack overflow we would need to reset the controller for
  // obvious reasons
}

void heap_caps_alloc_failed_hook(size_t requested_size, uint32_t caps,
                                 const char *function_name) {
  esp_rom_printf("Heap Full: \"%s\" was called but failed to allocate %d bytes "
                 "with 0x%lX capabilities.",
                 function_name, requested_size, caps);
  post_iotcore_error_event(MEMORY_ALLOCATION_FAILED, (void *)function_name,
                           sizeof(function_name));
}

/**
 * Purpose of this task is to serve as the task that will report statistics.
 * Some functionality like freeheap checks, reset reason check, uptime post,
 * reset count post, will be done here
 */
void system_monitoring_task(void *pvParameters) {
  // Check reset reason
  esp_reset_reason_t reset_reason = esp_reset_reason();
  post_iotcore_app_event(SYSTEM_RESET_REASON, &reset_reason,
                         sizeof(esp_reset_reason_t));
  // Get reset count, Update reset count
  int32_t restart_counter = 0;
  readKeyValueInFlash_int32("restart_counter", &restart_counter);
  /// TODO: Reset reset count after firmware version change. Requirement of
  /// canary
  // Only update counter if it was unintentional restart
  if (reset_reason != ESP_RST_POWERON && reset_reason != ESP_RST_EXT &&
      reset_reason != ESP_RST_SW && reset_reason != ESP_RST_DEEPSLEEP) {
    restart_counter++;
    restart_counter != 0
        ? post_iotcore_app_event(SYSTEM_RESET_COUNT, &restart_counter,
                                 sizeof(int32_t))
        : 0;
    saveKeyValueInFlash_int32("restart_counter", restart_counter);
  }

  while (1) {
    // Get uptime
    uint32_t uptime = esp_timer_get_time() / 1000;
    post_iotcore_app_event(SYSTEM_UPTIME_MS, &uptime, sizeof(uint32_t));

    // Get free heap
    size_t freeHeap = esp_get_free_heap_size();
    post_iotcore_app_event(SYSTEM_FREE_HEAP_BYTES, &freeHeap, sizeof(size_t));
    vTaskDelay(pdMS_TO_TICKS(60000)); // Post every minute. Other side will
                                      // decide when to publish and when not to.
  }

  vTaskDelete(NULL);
}

//////////Section Related to goodby protocol /////////////
/**
 * This portion deals with status info's when the system says it wants to go
 * into sleep due to some reason Current implementation is on voltage level of
 * battery
 */
#define STR_HELPER(x) #x
#define STR(x) STR_HELPER(x)
#define MINUTES_STAY_IN_GOODBY_MOOD 30
// define ext_wakeup_pins_mask in app_main based on the pins are being used for
// button e.g. const uint64_t ext_wakeup_pins_mask = BIT64(USER_BUTTON_0); const
// uint64_t ext_wakeup_pins_mask = 0; if button available

// Temporarily disable this function for C3 chip.
// Need to FIX Later
#if !CONFIG_IDF_TARGET_ESP32C3
void goToDeepSleep(const uint64_t ext_wakeup_pins_mask) {
  const int wakeup_time_sec = MINUTES_STAY_IN_GOODBY_MOOD * 60;
  printf("Enabling timer wakeup, %ds\n", wakeup_time_sec);
  esp_sleep_enable_timer_wakeup(wakeup_time_sec * 1000000);

  if (ext_wakeup_pins_mask != 0)
    esp_sleep_enable_ext1_wakeup(ext_wakeup_pins_mask, ESP_EXT1_WAKEUP_ALL_LOW);

  esp_deep_sleep_start();
}
#endif

void StartGoodByProtocol(float voltage, int32_t clientID,
                         int32_t *GOODByProtocolInInit,
                         const uint64_t ext_wakeup_pins_mask) {
  !clientID ? 0 : ({ return; });
  cJSON *root;
  cJSON *object;
  root = cJSON_CreateObject();
  char *json_str;
  char *device_id = NULL;
  asprintf(&device_id, "%ld", clientID);
  cJSON_AddItemToObject(root, "did", cJSON_CreateString(device_id));
  free(device_id);
#ifdef CONFIG_FW_IS_RC
  cJSON_AddItemToObject(
      root, "fv",
      cJSON_CreateString(
          "v" STR(CONFIG_CURRENT_FIRMWARE_MAJOR_VERSION) "." STR(CONFIG_CURRENT_FIRMWARE_MINOR_VERSION) "." STR(
              CONFIG_CURRENT_FIRMWARE_SUB_VERSION) "-rc" STR(CONFIG_FW_RC_NUM)));
#else
  cJSON_AddItemToObject(
      root, "fv",
      cJSON_CreateString("v" STR(CONFIG_CURRENT_FIRMWARE_MAJOR_VERSION) "." STR(
          CONFIG_CURRENT_FIRMWARE_MINOR_VERSION) "." STR(CONFIG_CURRENT_FIRMWARE_SUB_VERSION)));
#endif
  object = cJSON_CreateObject();
  cJSON_AddItemToObject(root, "goodby", object);
  myJSON_AddRawToObject(object, "vbat", (voltage), 3);

  json_str = cJSON_Print(root);
  post_mqtt_publish_event(json_str, strlen(json_str), "debugv0", 0, 1);
  /// TODO: Add this to slack
  *GOODByProtocolInInit = 1;
  saveKeyValueInFlash_int32("goodby", 1);
  if (json_str)
    free(json_str);
  cJSON_Delete(root);

  goToDeepSleep(ext_wakeup_pins_mask);
}

void EndGoodByProtocol(float voltage, int32_t *GOODByProtocolInInit) {
  saveKeyValueInFlash_int32("goodby", 0);

  *GOODByProtocolInInit = 0;
}
//////////  Goodby protocol section end  //////////////

void start_system_monitoring() {
  heap_caps_register_failed_alloc_callback(heap_caps_alloc_failed_hook);
  xTaskCreate(system_monitoring_task, "system_monitoring_task", 2048, NULL, 10,
              NULL);
}