#include "system_status_reporting.h"
#include "cJSON.h"
#include "iotcore_events.h"
#include "string.h"

#define HEAP_LOW_LOW_PRIORITY (50 * 1024)
#define HEAP_LOW_HIGH_PRIORITY (30 * 1024)
#define STR_HELPER(x) #x
#define STR(x) STR_HELPER(x)
bool free_heap_alert_sent_flag = false;
void system_status_report_handler(void *arg, esp_event_base_t event_base,
                                  int32_t event_id, void *event_data) {
  !arg ? 0 : ({ return; });
  cJSON *root;
  root = cJSON_CreateObject();
  char *json_str = NULL;
  char *device_id = NULL;
  asprintf(&device_id, "%ld", *(int32_t *)arg);
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
  switch (event_id) {
  case SYSTEM_RESET_COUNT:
    cJSON_AddItemToObject(root, "resetC",
                          cJSON_CreateNumber(*(int *)event_data));
    break;
  case SYSTEM_RESET_REASON:
    cJSON_AddItemToObject(root, "rs_code",
                          cJSON_CreateNumber(*(int *)event_data));
    break;
  case SYSTEM_UPTIME_MS:
    cJSON *object = cJSON_CreateObject();
    cJSON_AddItemToObject(root, "RunT", object);
    cJSON_AddItemToObject(object, "RunT",
                          cJSON_CreateNumber(*(int *)event_data));
    cJSON_AddItemToObject(object, "resetC", cJSON_CreateNumber(0));
    break;
  case SYSTEM_FREE_HEAP_BYTES:
    int freeheap = *(int *)event_data;
    if (freeheap < HEAP_LOW_HIGH_PRIORITY && !free_heap_alert_sent_flag) {
      cJSON_AddNumberToObject(root, "heaplow", freeheap);
      free_heap_alert_sent_flag = true;
    } else if (freeheap > HEAP_LOW_LOW_PRIORITY && free_heap_alert_sent_flag) {
      cJSON_AddNumberToObject(root, "heap", freeheap);
      free_heap_alert_sent_flag = false;
    } else {
      goto no_send;
    }
    break;
  case MQTT_DISCONNECT_COUNT_UPDATE:
    cJSON_AddItemToObject(root, "mqtt_dc",
                          cJSON_CreateNumber(*(int *)event_data));
  default:
    break; // Not my event
  }

  json_str = cJSON_Print(root);
  post_mqtt_publish_event(json_str, strlen(json_str), "debugv0", 0, 1);
  /// TODO: Add http post to slack here
no_send:
  if (json_str != NULL) {
    free(json_str);
  }
  cJSON_Delete(root);
}

void start_system_status_reporting(int32_t *clientID) {
  register_iotcore_app_event(SYSTEM_RESET_COUNT, system_status_report_handler,
                             clientID);
  register_iotcore_app_event(SYSTEM_RESET_REASON, system_status_report_handler,
                             clientID);
  register_iotcore_app_event(SYSTEM_UPTIME_MS, system_status_report_handler,
                             clientID);
  register_iotcore_app_event(SYSTEM_FREE_HEAP_BYTES,
                             system_status_report_handler, clientID);
  register_iotcore_app_event(MQTT_DISCONNECT_COUNT_UPDATE,
                             system_status_report_handler, clientID);
}