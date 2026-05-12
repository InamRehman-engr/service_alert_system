#include "wifi_manager.h"
#include "esp_timer.h"
#include "iotcore_events.h"
#ifdef CONFIG_ENABLE_NVS
#include "nvs_read_write.h"
#endif
#include "sysinfo.h"
#include "wifi_manager_provisioning_api.h"
#include "wifi_utils.h"

const char *TAG = "Wifi Manager";

static const char *wifi_manager_default_sta_ssid = "Iotcore Test";
static const char *wifi_manager_default_sta_pass = "12345677";

uint8_t numberOfConnectionRetries = 3;

// Used variables
uint8_t numberOfKnownNetworks = 0;
uint8_t filtered_list_connection_count = 0;

wifi_ssid_t *wifiConnectionList = NULL;
wifi_ssid_t wifiConnectionList_Filtered[20];

TaskHandle_t *scanTaskHandle = NULL;
TaskHandle_t *wifi_manager_appHandle = NULL;
wifi_ap_record_t scannedAPs[20];
uint16_t numberOfScannedAPs = 0;

SemaphoreHandle_t wifi_scan_semaphore =
    NULL; // Need this becuase wifi cannot scan when connecting to a wifi

bool wifi_manager_ap_desired_state = false;
uint64_t wifi_manager_ap_off_state_called_start_time = 0;
uint64_t wifi_manager_ap_off_state_grace_period = 60 * 1000000;

// Event groub bits for wifi here.
EventGroupHandle_t s_wifi_event_group = NULL;
// const int WIFI_CONNECTED_BIT = BIT0;
const int WIFI_STA_STARTED = BIT1;
const int WIFI_STA_CONNECTED = BIT2;
const int WIFI_AP_CONNECTED_BIT = BIT3;
const int STA_GOT_IP_BIT = BIT4;
const int WIFI_SCAN_INPROGRESS_BIT = BIT5;

// Error bits
const int STA_AUTH_ERROR = BIT16;
const int STA_NOT_FOUND = BIT17;

void wifi_manager_app(void *pvParams);
// Helper defines here

// This one can allos aus to iterate over list and connect to wifi's. and then
// wait for the connection. If the wifi has IP then okay otherwise next wifi
#define stationhasIP()                                                         \
  (xEventGroupGetBits(s_wifi_event_group) & (STA_GOT_IP_BIT))
#define isWifiConnected(wait)                                                  \
  (xEventGroupGetBits(s_wifi_event_group) & (WIFI_STA_CONNECTED))

/**
 * These 2 here are defined. the user may override these to enable webserver
 */
void __attribute__((weak)) device_connected_to_ap_cb() {
  ESP_LOGI(TAG, "Device connected to AP");
}
void __attribute__((weak)) device_disconnected_from_ap_cb() {
  ESP_LOGI(TAG, "Device disconnected from AP");
}

void createConnectionList(wifi_ap_record_t *scannedAPs,
                          uint16_t numberOfScannedAPs,
                          wifi_ssid_t *knownNetworks) {
  int connectionCount = 0;
  for (int j = 0; j < numberOfKnownNetworks; j++) {
    for (int i = 0; i < numberOfScannedAPs; i++) {
      if (strcmp((char *)scannedAPs[i].ssid, knownNetworks[j].ssid) == 0) {
        wifiConnectionList_Filtered[connectionCount] = knownNetworks[j];
        connectionCount++;
        break; // Break after finding the first match for each known network
      }
    }
  }
  filtered_list_connection_count = connectionCount;
  for (int i = 0; i < connectionCount; i++) {
    ESP_LOGD(TAG, "Connect to '%s'", wifiConnectionList_Filtered[i].ssid);
  }
}
// This performs a circular left shift for wifiConnectionList to adjust
// priorities for wifi networks.
void rotateWifiList() {
  if (numberOfKnownNetworks > 1) {
    wifi_ssid_t tempList = *wifiConnectionList;
    memcpy(wifiConnectionList, wifiConnectionList + 1,
           sizeof(wifi_ssid_t) * (numberOfKnownNetworks - 1));
    wifiConnectionList[numberOfKnownNetworks - 1] = tempList;
    vTaskDelay(pdMS_TO_TICKS(1000));
    createConnectionList(scannedAPs, numberOfScannedAPs, wifiConnectionList);
    esp_wifi_disconnect();
  }
}

static void scan_done_handler(void) {
  char bssid_s[18];
  esp_wifi_scan_get_ap_num(&numberOfScannedAPs);
  esp_err_t ret = esp_wifi_scan_get_ap_records(&numberOfScannedAPs, scannedAPs);
  if (ret == ESP_OK) {
    for (int x = 0; x < numberOfScannedAPs; x++) {
      sprintf(bssid_s, "%02X:%02X:%02X:%02X:%02X:%02X", scannedAPs[x].bssid[0],
              scannedAPs[x].bssid[1], scannedAPs[x].bssid[2],
              scannedAPs[x].bssid[3], scannedAPs[x].bssid[4],
              scannedAPs[x].bssid[5]);
      ESP_LOGD(TAG, "- %s (%s)  %s %ddB", scannedAPs[x].ssid, bssid_s,
               get_rssi_bars(scannedAPs[x].rssi), scannedAPs[x].rssi);
    }
  }

  createConnectionList(scannedAPs, numberOfScannedAPs, wifiConnectionList);
}

void wifi_init_softap() {
  wifi_config_t ap_config = {0};
  ESP_LOGI(TAG, "Setting Wifi AP mode");
  char AP_SSID[50] = {0};
  char AP_pass[50] = {0};
  sprintf(AP_SSID, "IOTCORE_%s", getDeviceInfo()->registrationNumber);
  strcpy((char *)ap_config.ap.ssid, AP_SSID);
  strcpy((char *)ap_config.ap.password, AP_pass);
  ap_config.ap.ssid_len = 0;
  ap_config.ap.authmode = WIFI_AUTH_WPA_WPA2_PSK;
  ap_config.ap.max_connection = 4;
  ap_config.ap.channel = 3;
  ESP_LOGI(TAG, "ap_config.ap.channel = %d", ap_config.ap.channel);
  if (strlen((char *)ap_config.ap.password) == 0) {
    ap_config.ap.authmode = WIFI_AUTH_OPEN;
    ESP_LOGI(TAG, "ap_config.ap.authmode = WIFI_AUTH_OPEN");
  }
  ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_AP, &ap_config));

  ESP_LOGI(TAG, "wifi_init_softap finished.SSID:%s password:%s pin:%s",
           (char *)ap_config.ap.ssid, (char *)ap_config.ap.password,
           getDeviceInfo()->devicePin);
}

static void wifi_event_handler_STA_DISCONNECTED(void *arg, int32_t event_id,
                                                void *event_data) {
  // This event will be called when the wifi disconnects or fails to connect.
  // Better handling of this will allow the wifi manager to filter through APs
  // that cannot be connected to and connect to a valid one.
  const wifi_event_sta_disconnected_t *disconnected =
      (wifi_event_sta_disconnected_t *)event_data;
  ESP_LOGE(TAG, "%s: Wifi Disconnected: %s", __func__, disconnected->ssid);

  // Set bits indicating disconnection
  xEventGroupClearBits(s_wifi_event_group, STA_GOT_IP_BIT);
  xEventGroupClearBits(s_wifi_event_group, WIFI_STA_CONNECTED);
  switch (disconnected->reason) {

  case WIFI_REASON_CONNECTION_FAIL:
  case WIFI_REASON_HANDSHAKE_TIMEOUT:
  case WIFI_REASON_4WAY_HANDSHAKE_TIMEOUT:
    xEventGroupSetBits(s_wifi_event_group, STA_AUTH_ERROR);
    break;
  case WIFI_REASON_NO_AP_FOUND:
    // Somehow we misssed this in filter of known aps after scan.
    xEventGroupSetBits(s_wifi_event_group, STA_NOT_FOUND);

    break;
  case WIFI_REASON_BEACON_TIMEOUT:
    break;
  case WIFI_REASON_ASSOC_LEAVE:
    break;
  default:
    break;
  }
}

static void wifi_event_handler_STA_GOT_IP(void *arg, int32_t event_id,
                                          void *event_data) {
  const ip_event_got_ip_t *got_ip = (ip_event_got_ip_t *)event_data;

  ESP_LOGD(TAG, "SYSTEM_EVENT_STA_GOT_IP got ip:" IPSTR,
           IP2STR(&got_ip->ip_info.ip));
  xEventGroupSetBits(s_wifi_event_group, STA_GOT_IP_BIT);
}

static void wifi_event_handler_STA_CONNECTED(void *arg, int32_t event_id,
                                             void *event_data) {
  ESP_LOGI(TAG, "WIFI_Connected");
  xEventGroupSetBits(s_wifi_event_group, WIFI_STA_CONNECTED);
}
static void wifi_event_handler(void *arg, esp_event_base_t event_base,
                               int32_t event_id, void *event_data) {
  ESP_LOGD(TAG, "WIFI_EVENT");
  switch (event_id) {
  case WIFI_EVENT_WIFI_READY:
    ESP_LOGD(TAG, "WIFI_EVENT_WIFI_READY");
    break;

  case WIFI_EVENT_SCAN_DONE:

    scan_done_handler();
    break;
  case WIFI_EVENT_STA_START:
    xEventGroupSetBits(s_wifi_event_group, WIFI_STA_STARTED);
    break;
  case WIFI_EVENT_STA_STOP:
    xEventGroupClearBits(s_wifi_event_group, WIFI_STA_STARTED);
    break;
  case WIFI_EVENT_STA_CONNECTED:
    wifi_event_handler_STA_CONNECTED(arg, event_id, event_data);
    break;
  case WIFI_EVENT_STA_DISCONNECTED:
    wifi_event_handler_STA_DISCONNECTED(arg, event_id, event_data);
    break;
  case WIFI_EVENT_AP_START:
    // wifi_event_handler_AP_STARTED(NULL, event_id, event_data);
    break;
  case WIFI_EVENT_AP_STOP:
    // wifi_event_handler_AP_STOPED(NULL, event_id, event_data);
    break;
  case WIFI_EVENT_AP_STACONNECTED:
    const wifi_event_ap_staconnected_t *sta_connected =
        (wifi_event_ap_staconnected_t *)event_data;
    ESP_LOGI(TAG, "station:" MACSTR " join, AID=%hhd",
             MAC2STR(sta_connected->mac), sta_connected->aid);
    device_connected_to_ap_cb();
    break;
  case WIFI_EVENT_AP_STADISCONNECTED:
    const wifi_event_ap_stadisconnected_t *sta_disconnected =
        (wifi_event_ap_stadisconnected_t *)event_data;
    ESP_LOGI(TAG, "station:" MACSTR "leave, AID=%d",
             MAC2STR(sta_disconnected->mac), sta_disconnected->aid);
    wifi_sta_list_t sta1;
    esp_wifi_ap_get_sta_list(&sta1);
    ESP_LOGI(TAG, "Number of connected STAs: %d\n", sta1.num);
    sta1.num == 0 ? device_disconnected_from_ap_cb()
                  : 0; // This will make sure Webserver is turned off when no
                       // devices are connected
    break;
  default:
    break;
  }
}

static void ip_event_handler(void *arg, esp_event_base_t event_base,
                             int32_t event_id, void *event_data) {
  switch (event_id) {
  // Got Ip Handler here. This will be used by wifi manager to check if the
  // connected wifi passed DHCP enumeration
  case IP_EVENT_STA_GOT_IP:
    wifi_event_handler_STA_GOT_IP(arg, event_id, event_data);
    break;
  default:
    break;
  }
}

void readWifiListFromNVS(void) {
  size_t readLen = 0;
#ifdef CONFIG_ENABLE_NVS
  if (readKeyValueInFlash_blob("wifi_ssid_t", NULL, &readLen) == ESP_OK) {
    numberOfKnownNetworks = readLen / sizeof(wifi_ssid_t);
    if (wifiConnectionList == NULL)
      wifiConnectionList =
          malloc((int)(readLen / sizeof(wifi_ssid_t)) * sizeof(wifi_ssid_t));
    else
      wifiConnectionList =
          realloc((void *)wifiConnectionList,
                  (int)(readLen / sizeof(wifi_ssid_t)) * sizeof(wifi_ssid_t));
    readKeyValueInFlash_blob("wifi_ssid_t", (uint8_t *)wifiConnectionList,
                             &readLen);
  } else {
    ESP_LOGW(TAG, "Nothing in Flash");
    if (wifiConnectionList == NULL)
      wifiConnectionList = malloc((int)(sizeof(wifi_ssid_t)));
    else
      wifiConnectionList =
          realloc((void *)wifiConnectionList, (int)(sizeof(wifi_ssid_t)));
    wifiConnectionList[0].priority = 0;
    strcpy(wifiConnectionList[0].ssid, wifi_manager_default_sta_ssid);
    strcpy(wifiConnectionList[0].password, wifi_manager_default_sta_pass);
    numberOfKnownNetworks = 1;

#ifdef CONFIG_IOTCORE_DEFAULT_WIFI
    wifiConnectionList =
        realloc((void *)wifiConnectionList,
                (numberOfKnownNetworks + 1) * (sizeof(wifi_ssid_t)));
    wifiConnectionList[1].priority = 1;
    strcpy(wifiConnectionList[1].ssid, CONFIG_IOTCORE_WIFI_DEFAULT_SSID);
    strcpy(wifiConnectionList[1].password, CONFIG_IOTCORE_WIFI_DEFAULT_PASS);
    numberOfKnownNetworks = 2;
#endif
    // Nothing in flash. Preferrably start in AP mode.
    // Add Cowlar Test here.
  }
#endif
}

void storeWifiListInNVS(wifi_ssid_t *List, int numberofsta) {
#ifdef CONFIG_ENABLE_NVS
  saveKeyValueInFlash_blob("wifi_ssid_t", (uint8_t *)List,
                           numberofsta * sizeof(wifi_ssid_t));
#endif
}

void updateWifiList(cJSON *ssids_json) {
  // This function expects a cJSON list.
  // This is here to keep compatibility
  // The list will contain preferences as many as there are wifi's in the list.
  // 2 wifi's cannot have the same preference. that will have unforseen problems

  // This function needs to be called on a lock where this structure cannot be
  // accessed while being updated.
  /// TODO: Add lock
  cJSON *item;
  cJSON *obj;
  bool add_default = true;
  bool add_configured_wifi = true;
  if (cJSON_IsArray(ssids_json)) {
    uint8_t size = cJSON_GetArraySize(ssids_json);
    printf("Total %d wifi are in list \n", size);
    numberOfKnownNetworks = size;
    if (wifiConnectionList == NULL)
      wifiConnectionList = malloc(size * sizeof(wifi_ssid_t));
    else
      wifiConnectionList =
          realloc((void *)wifiConnectionList, size * sizeof(wifi_ssid_t));
    for (int i = 0; i < size; i++) {
      item = cJSON_GetArrayItem(ssids_json, i);
      obj = cJSON_GetObjectItem(item, "n");
      uint8_t preference = obj->valueint;

      wifiConnectionList[preference].priority = obj->valueint;
      obj = cJSON_GetObjectItem(item, "s");
      if (obj != NULL)
        strcpy(wifiConnectionList[preference].ssid, obj->valuestring);
      if (strcmp(wifiConnectionList[preference].ssid,
                 wifi_manager_default_sta_ssid) == 0)
        add_default = false;
#ifdef CONFIG_IOTCORE_DEFAULT_WIFI
      if (strcmp(wifiConnectionList[preference].ssid,
                 CONFIG_IOTCORE_WIFI_DEFAULT_SSID) == 0)
        add_configured_wifi = false;
#endif
      obj = cJSON_GetObjectItem(item, "p");
      if (obj != NULL)
        strcpy(wifiConnectionList[preference].password, obj->valuestring);
      else
        strcpy(wifiConnectionList[preference].password, "\0");
      printf("[%d] SSID %30s  Pass %10s\n", preference,
             wifiConnectionList[preference].ssid,
             wifiConnectionList[preference].password);
    }
#ifdef CONFIG_IOTCORE_DEFAULT_WIFI
    if (add_configured_wifi) {
      wifiConnectionList =
          realloc((void *)wifiConnectionList, (size + 1) * sizeof(wifi_ssid_t));
      strcpy(wifiConnectionList[size].ssid, CONFIG_IOTCORE_WIFI_DEFAULT_SSID);
      strcpy(wifiConnectionList[size].password,
             CONFIG_IOTCORE_WIFI_DEFAULT_PASS);
      wifiConnectionList[size].priority = size;
      size++;
    }
#endif
    if (add_default) {
      wifiConnectionList =
          realloc((void *)wifiConnectionList, (size + 1) * sizeof(wifi_ssid_t));
      strcpy(wifiConnectionList[size].ssid, wifi_manager_default_sta_ssid);
      strcpy(wifiConnectionList[size].password, wifi_manager_default_sta_pass);
      wifiConnectionList[size].priority = size;
      size++;
    }
    storeWifiListInNVS(wifiConnectionList, size);
  } else {
    // error here for json parsing
  }
}

void scanTask(void *pvParams) {
  wifi_scan_config_t scan_config = {0};
  while (true) {
    ESP_LOGD(TAG, "wifi_scan_start");
    // WifiScanInProgress();
    esp_err_t err = ESP_FAIL;
    int retry = 4;
    while (err == ESP_FAIL && retry-- > 0) {
      if (xEventGroupGetBits(s_wifi_event_group) & WIFI_STA_STARTED) {
        err = esp_wifi_scan_start(&scan_config, true);
      } else {
        err = ESP_ERR_WIFI_NOT_STARTED;
        ESP_LOGE(TAG, "ESP_ERR_WIFI_NOT_STARTED");
      }

      if (err == ESP_OK) {
        // WifiScanDone();
        // return err;
      }
      vTaskDelay(pdMS_TO_TICKS(100));
    }
    // WifiScanDone();
    vTaskDelay(pdMS_TO_TICKS(WIFI_STA_SCAN_INTERVAL));
  }
}

void init_wifi_manager() {
  if (s_wifi_event_group == NULL) {
    s_wifi_event_group = xEventGroupCreate();
  }
  readWifiListFromNVS();

  esp_netif_create_default_wifi_sta();
  esp_netif_create_default_wifi_ap();
  wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
  esp_wifi_init(&cfg);
  // Only wifi event here. IP event in main connectivity
  ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID,
                                             &wifi_event_handler, NULL));
  ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP,
                                             &ip_event_handler, NULL));
  // Initialize softap but not start it.
  wifi_init_softap();
}

void start_wifi_manager() {
  // Start will only work if not already started. Logic is to check for
  // taskhandles
  if (scanTaskHandle == NULL && wifi_manager_appHandle == NULL) {
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    cfg.nvs_enable = false;
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));
    ESP_ERROR_CHECK(esp_wifi_set_ps(WIFI_PS_MAX_MODEM));
    ESP_ERROR_CHECK(esp_wifi_set_storage(WIFI_STORAGE_RAM));
    esp_wifi_set_mode(WIFI_MODE_APSTA);
    ESP_ERROR_CHECK(esp_wifi_start());
    ESP_LOGD(TAG, "Starting Tasks");
    vTaskDelay(pdMS_TO_TICKS(1000));
    scanTaskHandle = malloc(sizeof(TaskHandle_t));
    wifi_manager_appHandle = malloc(sizeof(TaskHandle_t));
    xTaskCreate(scanTask, "scanTask", 512 * 3, NULL, 10, scanTaskHandle);
    xTaskCreate(wifi_manager_app, "wifi_manager_app", 1024 * 2.5, NULL, 10,
                wifi_manager_appHandle);
  }
}

void stop_wifi_manager() {
  if (scanTaskHandle != NULL && wifi_manager_appHandle != NULL) {
    esp_wifi_disconnect();
    vTaskDelete(*scanTaskHandle);
    vTaskDelete(*wifi_manager_appHandle);
    free(scanTaskHandle);
    free(wifi_manager_appHandle);
    scanTaskHandle = NULL;
    wifi_manager_appHandle = NULL;
    esp_wifi_deinit();
  }
}

void connectToWifi(wifi_ssid_t wifi) {
  wifi_config_t sta_config = {0};
  strcpy((char *)sta_config.sta.ssid, wifi.ssid);
  strcpy((char *)sta_config.sta.password, wifi.password);
  ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_STA, &sta_config));
  esp_wifi_set_ps(DEFAULT_PS_MODE);
  esp_wifi_connect();
}

void wifi_manager_app(void *pvParams) {
  // Main task here
  esp_wifi_set_mode(WIFI_MODE_APSTA);
  wifi_mode_t current_mode;
  while (true) {
    if (filtered_list_connection_count !=
        0) // Check if first scan has completed and network has been found
    {
      if (!isWifiConnected(false)) {
        for (int x = 0; x < filtered_list_connection_count; x++) {
          for (int y = 0; y < numberOfConnectionRetries; y++) {
            connectToWifi(wifiConnectionList_Filtered[x]);
            EventBits_t uxbits = xEventGroupWaitBits(
                s_wifi_event_group,
                STA_AUTH_ERROR | WIFI_STA_CONNECTED | STA_NOT_FOUND, false,
                false, pdMS_TO_TICKS(10000));
            if (uxbits & STA_AUTH_ERROR) {
              ESP_LOGE(TAG, "Skipping connection retries due to auth error");
              xEventGroupClearBits(s_wifi_event_group, STA_AUTH_ERROR);
              break; // Goto next wifi
            }
            if (uxbits &
                WIFI_STA_CONNECTED) // Station got connected. Wait for Ip
            {
              vTaskDelay(pdMS_TO_TICKS(3000));
            }
            if (uxbits & STA_NOT_FOUND) {
              xEventGroupClearBits(s_wifi_event_group, STA_NOT_FOUND);
              esp_wifi_disconnect();
              // AP was not found
              // Trigger a scan here.
              break;
            }

            if (stationhasIP()) {
              // Station has IP break from it.
              break;
            }
          }
          if (stationhasIP()) {
            break;
          }
        }
      }
    } else {
      // Connect to Default wifi and start AP here.
      esp_wifi_set_mode(WIFI_MODE_APSTA);
    }
    esp_wifi_get_mode(&current_mode);
    // This condition makes sure esp is in AP mode if there is no valid wifi in
    // list. then it can be turned off
    if (!wifi_manager_ap_desired_state && current_mode == WIFI_MODE_APSTA &&
        filtered_list_connection_count != 0) {
      if (esp_timer_get_time() > (wifi_manager_ap_off_state_called_start_time +
                                  wifi_manager_ap_off_state_grace_period)) {
        ESP_LOGW(TAG, "AP is turned off");
        esp_wifi_set_mode(WIFI_MODE_STA);
      }
    } else if (wifi_manager_ap_desired_state) {
      ESP_LOGW(TAG, "AP is turned on");
      esp_wifi_set_mode(WIFI_MODE_APSTA);
    }
    vTaskDelay(pdMS_TO_TICKS(10000));
  }
}

wifi_ap_record_t *api_wifi_manager_get_scan_list(uint8_t *number) {
  *number = numberOfScannedAPs;
  return scannedAPs;
}

/**
 * This overrides the current runtime config
 */
void api_wifi_manager_connect_to_wifi(char *ssid, char *password) {
  cJSON *jsonArray = cJSON_CreateArray();
  cJSON *item1 = cJSON_CreateObject();
  int i = 0;
  cJSON_AddStringToObject(item1, "s", ssid);
  cJSON_AddStringToObject(item1, "p", password);
  cJSON_AddNumberToObject(item1, "n", i);
  cJSON_AddItemToArray(jsonArray, item1);
  i++;

#ifdef CONFIG_IOTCORE_DEFAULT_WIFI
  cJSON *item2 = cJSON_CreateObject();
  cJSON_AddStringToObject(item2, "s", CONFIG_IOTCORE_WIFI_DEFAULT_SSID);
  cJSON_AddStringToObject(item2, "p", CONFIG_IOTCORE_WIFI_DEFAULT_PASS);
  cJSON_AddNumberToObject(item2, "n", i);
  cJSON_AddItemToArray(jsonArray, item2);
  i++;
#endif

  cJSON *item3 = cJSON_CreateObject();
  cJSON_AddStringToObject(item3, "s", wifi_manager_default_sta_ssid);
  cJSON_AddStringToObject(item3, "p", wifi_manager_default_sta_pass);
  cJSON_AddNumberToObject(item3, "n", i);
  cJSON_AddItemToArray(jsonArray, item3);
  i++;

  updateWifiList(jsonArray);
  cJSON_Delete(jsonArray);
  createConnectionList(scannedAPs, numberOfScannedAPs, wifiConnectionList);
  esp_wifi_disconnect();
}

/**
 * This will get rid of the corresponding cJSON entity provided.
 *
 */
void api_wifi_manager_update_known_network_list(cJSON *list) {
  wifi_ap_record_t ap_info;
  esp_wifi_sta_get_ap_info(&ap_info);
  char *current_wifi_ssid = (char *)ap_info.ssid;
  char *current_wifi_password = NULL;
  for (uint8_t i = 0; i < numberOfKnownNetworks; i++) {
    if (strcmp((char *)ap_info.ssid, wifiConnectionList[i].ssid) == 0) {
      current_wifi_password = wifiConnectionList[i].password;
      break;
    }
  }

  cJSON *updated_list = cJSON_Duplicate(list, true);

  // Check if the current WiFi is already in the list
  bool current_wifi_in_list = false;
  cJSON *wifi = NULL;
  uint8_t i = 0;
  uint8_t priority;
  cJSON_ArrayForEach(wifi, updated_list) {
    priority = (uint8_t)cJSON_GetObjectItem(wifi, "n")->valueint;
    if (priority != i++) {
      ESP_LOGE(TAG, "WiFi List invalid. Failed to parse");
      return;
    }
    cJSON *ssid = cJSON_GetObjectItem(wifi, "s");
    if (ssid == NULL)
      continue;
    if (strcmp(ssid->valuestring, current_wifi_ssid) == 0) {
      current_wifi_in_list = true;
      break;
    }
    cJSON_ReplaceItemInObjectCaseSensitive(wifi, "n",
                                           cJSON_CreateNumber(priority + 1));
  }

  // If the current WiFi is not found in the list, add it to the list
  if (!current_wifi_in_list) {
#ifdef CONFIG_ENABLE_NVS
    char *json_str = NULL;
    char *topic = NULL;
    int32_t clientID;
    readKeyValueInFlash_int32("clientId", &clientID);
    asiprintf(&topic, "d/%ld/network/wifi/list", clientID);

    ESP_LOGW("TAG", "Adding new wifi in list");
    cJSON *new_item = cJSON_CreateObject();
    cJSON_AddStringToObject(new_item, "s",
                            current_wifi_ssid); // Add SSID
    cJSON_AddStringToObject(new_item, "p",
                            current_wifi_password); // Add password
    cJSON_AddNumberToObject(new_item, "n", 0);

    cJSON_InsertItemInArray(updated_list, 0, new_item);

    updateWifiList(updated_list);
    json_str = cJSON_PrintUnformatted(updated_list);
    post_mqtt_publish_event(json_str, strlen(json_str), topic, true, 2);

    free(topic);
    free(json_str);
#endif
  } else {
    updateWifiList(list);
  }

  cJSON_Delete(list);
  cJSON_Delete(updated_list);

  createConnectionList(scannedAPs, numberOfScannedAPs, wifiConnectionList);
}

/**
 * True to enable provisioning mode
 * False to disable
 */
void api_wifi_manager_provisioning_mode(bool mode) {
  wifi_manager_ap_desired_state = mode;
  !mode ? wifi_manager_ap_off_state_called_start_time = esp_timer_get_time()
        : 0;
}