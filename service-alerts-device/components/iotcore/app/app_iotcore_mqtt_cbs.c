#include "app_iotcore_mqtt_cbs.h"

#include "cJSON.h"
#include "esp_log.h"
#include "esp_types.h"
#include "string.h"

#include "iotcore_events.h"
#include "sysinfo.h"

#include "app_iotcore_ota_event_handler.h"
#include "firmware_check.h"
#ifdef CONFIG_ENABLE_OTA
#include "http_ota_handler.h"
#endif

void mqtt_s_ota_cb(char *topic, size_t topiclen, char *data, size_t datalen) {
#ifdef CONFIG_ENABLE_OTA
  cJSON *root = cJSON_Parse(data);
  if (root == NULL) {
    ESP_LOGE("ota", "Failed to parse JSON data");
    return;
  }

  cJSON *json_object = root;

  if (cJSON_IsArray(root)) {
    int array_size = cJSON_GetArraySize(root);
    if (array_size != 1) {
      ESP_LOGE("ota",
               "Multiple objects (%d) received. Proceeding with 1st object",
               array_size);
    }
    json_object = cJSON_GetArrayItem(root, 0);
    if (json_object == NULL) {
      ESP_LOGE("ota", "Failed to get the 1st object from JSON array");
      cJSON_Delete(root);
      return;
    }
  }

  cJSON *value = cJSON_GetObjectItem(json_object, "url");
  if (value == NULL || !cJSON_IsString(value)) {
    ESP_LOGE("ota", "Failed to get 'url' from JSON data");
    cJSON_Delete(root);
    return;
  }

  cJSON *delta_flag = cJSON_GetObjectItem(json_object, "delta");
  bool delta_firmware = delta_flag ? cJSON_IsTrue(delta_flag) : false;

  cJSON *hwv = cJSON_GetObjectItem(json_object, "hwv");
  if (hwv == NULL || !cJSON_IsString(hwv)) {
    ESP_LOGE("ota", "Failed to get 'hwv' from JSON data");
    cJSON_Delete(root);
    return;
  }

  cJSON *version = cJSON_GetObjectItem(json_object, "version");
  if (version == NULL || !cJSON_IsString(version)) {
    ESP_LOGE("ota", "Failed to get 'version' from JSON data");
    cJSON_Delete(root);
    return;
  }

  cJSON *rb = cJSON_GetObjectItem(json_object, "rb");
  if (rb == NULL || !cJSON_IsBool(rb)) {
    ESP_LOGE("ota", "Failed to get 'rb' from JSON data");
    cJSON_Delete(root);
    return;
  }

  if (validate_hardware_version(hwv->valuestring,
                                getDeviceInfo()->hardwareVersion) ==
          HARDWARE_VERSION_OK &&
      validate_firmware_version(version->valuestring,
                                getDeviceInfo()->firmwareVersion,
                                cJSON_IsTrue(rb)) == FIRMWARE_VERSION_OK) {
    char *ota_topic = NULL;
    int extractedTopic;
    if (sscanf(topic, "d/%d/ota", &extractedTopic) != 1) {
      ESP_LOGE("ota", "Failed to extract topic");
      cJSON_Delete(root);
      return;
    }
    asiprintf(&ota_topic, "d/%d/ota", extractedTopic);
    if (ota_topic == NULL) {
      ESP_LOGE("ota", "Failed to allocate memory for ota_topic");
      cJSON_Delete(root);
      return;
    }
    init_ota_event_handler(ota_topic, version->valuestring);
    http_ota_handler_init(value->valuestring, delta_firmware);
    free(ota_topic);
  }
  cJSON_Delete(root);
#endif
}

#ifdef CONFIG_ENABLE_WIFI
#include "wifi_manager.h"
#include "wifi_manager_provisioning_api.h"
#endif
void mqtt_s_wifi_scan_cb(char *topic, size_t topiclen, char *data,
                         size_t datalen) {

#ifdef CONFIG_ENABLE_WIFI
  char *new_topic = NULL;
  int deviceId = 0;
  sscanf(topic, "d/%d/network/wifi/scan", &deviceId);
  asiprintf(&new_topic, "d/%d/network/wifi/scanned", deviceId);

  uint8_t numberOfScanAPs = 0;
  wifi_ap_record_t *wifi_List =
      api_wifi_manager_get_scan_list(&numberOfScanAPs);
  cJSON *response;
  cJSON *ssids_json = NULL;
  cJSON *ssid_json = NULL;
  char *json_str;
  response = cJSON_CreateObject();
  cJSON_AddItemToObject(response, "response", cJSON_CreateString("success"));
  ssids_json = cJSON_CreateArray();
  cJSON_AddItemToObject(response, "SSIDS", ssids_json);

  for (int i = 0; i < numberOfScanAPs; i++) {
    ssid_json = cJSON_CreateObject();
    cJSON_AddItemToArray(ssids_json, ssid_json);
    cJSON_AddItemToObject(ssid_json, "ssid",
                          cJSON_CreateString((char *)wifi_List[i].ssid));
    cJSON_AddItemToObject(ssid_json, "rssi",
                          cJSON_CreateNumber(wifi_List[i].rssi));
  }
  json_str = cJSON_PrintUnformatted(response);

  cJSON_Delete(response);

  post_mqtt_publish_event(json_str, strlen(json_str), new_topic, false, 2);
  free(new_topic);
  free(json_str);
#endif
}

void mqtt_s_wifi_list_cb(char *topic, size_t topiclen, char *data,
                         size_t datalen) {

#ifdef CONFIG_ENABLE_WIFI
  char *mydata = malloc(datalen + 1);
  memcpy(mydata, data, datalen);
  mydata[datalen] = '\0';
  cJSON *ssids_json = cJSON_Parse(mydata);
  api_wifi_manager_update_known_network_list(ssids_json);
  free(mydata);
#endif
}

#include "system_partition.h"
void mqtt_set_boot_partition_cb(char *topic, size_t topiclen, char *data,
                                size_t datalen) {
  char *mydata = malloc(datalen + 1);
  memcpy(mydata, data, datalen);
  mydata[datalen] = '\0';
  if (setBootPartition(mydata) != ESP_OK) {
    ESP_LOGE("mqtt_set_boot_partition_cb", "setBootPartition failed");
  }
}
void mqtt_reset_device_cb(char *topic, size_t topiclen, char *data,
                          size_t datalen) {
  if (data == NULL) {
    ESP_LOGW("mqtt_reset_device_cb", "data is NULL");
    return;
  }
  char *mydata = malloc(datalen + 1);
  memcpy(mydata, data, datalen);
  mydata[datalen] = '\0';
  if (strcmp(mydata, getDeviceInfo()->registrationNumber) == 0) {
    ESP_LOGW("mqtt_reset_device_cb", "reset device");
    esp_restart();
  } else {
    ESP_LOGW("mqtt_reset_device_cb", "wrong registration number %s", data);
    free(mydata);
  }
}