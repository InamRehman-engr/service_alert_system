/**
 * Server expects some sort of update regarding current status of ota to display
 * on dashboard for visualization of what the progress of ota is if it even is
 * running or has it failed. We do that communication through mqtt. This will be
 * responsible to emit that information to iotcore(server).
 */

#include "app_iotcore_ota_event_handler.h"

#include "cJSON.h"
#include "string.h"

#include "iotcore_events.h"
#include "sysinfo.h"

/// TODO: change event to single event.
#ifdef CONFIG_ENABLE_OTA
#include "native_ota.h"
#endif

bool ota_event_listener_started = false;
char *topic = NULL;
char *newVer = NULL;
void ota_status_event_handler(void *event_handler_arg,
                              esp_event_base_t event_base, int32_t event_id,
                              void *event_data) {
#ifdef CONFIG_ENABLE_OTA
  char *deviceVersionInfo = NULL;
  native_ota_event_message_t native_ota_event_message =
      *(native_ota_event_message_t *)event_data;
  switch (native_ota_event_message.state) {
  case NATIVE_OTA_PROGRESS_UPDATE_EVENT:
    asprintf(&deviceVersionInfo,
             "{\"version\":\"%s\",\"hwv\":\"%s\",\"newv\":\"%s\",\"ota\":%d,"
             "\"p\":%.2f%%}",
             getDeviceInfo()->firmwareVersion, getDeviceInfo()->hardwareVersion,
             newVer, native_ota_event_message.state,
             native_ota_event_message.progress);
    break;
  case NATIVE_OTA_FAILED_EVENT:
    asprintf(&deviceVersionInfo,
             "{\"version\":\"%s\",\"hwv\":\"%s\",\"ota\":%d,\"r\":%d}",
             getDeviceInfo()->firmwareVersion, getDeviceInfo()->hardwareVersion,
             native_ota_event_message.state, native_ota_event_message.reason);
    break;
  case NATIVE_OTA_SUCCESFUL_EVENT:
    asprintf(&deviceVersionInfo,
             "{\"version\":\"%s\",\"hwv\":\"%s\",\"newv\":\"%s\",\"ota\":%d,"
             "\"status\":\"Completed\"}",
             getDeviceInfo()->firmwareVersion, getDeviceInfo()->hardwareVersion,
             newVer, native_ota_event_message.state);
    break;
  default:
    break;
  }
  if (deviceVersionInfo != NULL) {
    post_mqtt_publish_event(deviceVersionInfo, strlen(deviceVersionInfo), topic,
                            2, true);
    free(deviceVersionInfo);
  }
#endif
}

void init_ota_event_handler(char *topic_to_send_ota_alerts, char *newVersion) {

  if (ota_event_listener_started) {
    if (strcmp(newVer, newVersion) != 0) {
      newVer = realloc(newVer, sizeof(newVersion));
      strcpy(newVer, newVersion);
    }
    return;
  }
  ota_event_listener_started = true;
  asprintf(&topic, "%s", topic_to_send_ota_alerts);
  asprintf(&newVer, "%s", newVersion);
  register_iotcore_app_event(NATIVE_OTA_EVENT, ota_status_event_handler, NULL);
}