#include "app_iotcore.h"
#include "freertos/FreeRTOS.h"
#include "string.h"

#ifdef CONFIG_ENABLE_IOTCORE_EVENTS
#ifdef CONFIG_ENABLE_ERROR_HANDLING
#include "errorHandling.h"
#endif
#include "iotcore_events.h"
#ifdef CONFIG_ENABLE_SYSTEM_MONITORING
#include "system_monitoring.h"
#endif
#ifdef CONFIG_ENABLE_SYSTEM_STATUS_REPORTING
#include "system_status_reporting.h"
#endif
#endif
#ifdef CONFIG_ENABLE_NVS
#include "nvs_read_write.h"
#endif
#include "connectivity.h"
#include "connectivity_overrides.h"
#ifdef CONFIG_ENABLE_WIFI
#include "wifi_manager.h"
#endif

#include "app_iotcore_mqtt_pub_sub_events.h"
#ifdef CONFIG_ENABLE_MQTT
#include "app_mqtt.h"
#endif
#ifdef CONFIG_ENABLE_APP_RTC
#include "app_rtc.h"
#endif
#include "sysinfo.h"

#ifdef CONFIG_ENABLE_IOTCORE_SERVER_API
#include "iotcore_server_apis.h"
#endif

#include "app_iotcore_mqtt_cbs.h"
#include "app_iotcore_network_event_handler.h"

#ifdef CONFIG_IOTCORE_MQTT_DEFAULT_CLIENT
// Default mqtt_instance
struct mqtt_app_instance_t *mqtt_app_instance;
#endif
// Main init
#include "http_server_local.h"
#include "system_partition.h"

static const char *TAG = "app_iotcore";
httpd_handle_t webserver_handler = NULL;

void __attribute__((weak))
register_custom_webserver_handlers(httpd_handle_t webserver_handler) {
  ESP_LOGD(TAG, "Registering custom webserver handlers");
}
void device_connected_to_ap_cb() {
  if (webserver_handler == NULL) {
    webserver_handler = start_webserver();
    webserver_handler ? register_custom_webserver_handlers(webserver_handler)
                      : 0;
  }
}

void device_disconnected_from_ap_cb() {
  if (webserver_handler != NULL) {
    stop_webserver(webserver_handler);
    webserver_handler = NULL;
  }
}
#ifdef ALLOW_CUSTOM_INTERNET_AVAILABILITY_CHECK
void mqtt_connected_cb() { connectivity_override_internet_availability(true); }

void mqtt_disconnected_cb() {
  connectivity_override_internet_availability(false);
}
#endif

void mqtt_subscriptions_callback_iotcore(char *topic, size_t topiclen,
                                         char *data, size_t datalen) {
  char *mytopic = malloc(topiclen + 1);
  char *mydata = malloc(datalen + 1);
  memcpy(mytopic, topic, topiclen);
  memcpy(mydata, data, datalen);
  mytopic[topiclen] = '\0';
  mydata[datalen] = '\0';
  printf("Topic: %s, Data: %s\n", mytopic, mydata);
  free(mytopic);
  free(mydata);
}

void onboarding_Complete_cb(void *arg, esp_event_base_t event_base,
                            int32_t event_id, void *event_data) {
  int32_t clientID;
#ifdef CONFIG_ENABLE_NVS
  readKeyValueInFlash_int32("clientId", &clientID);
  if (*(int *)event_data != clientID) {
    saveKeyValueInFlash_int32("clientId", *(int *)event_data);
    esp_restart();
  }
#endif
}

void init_iotcore(iotcore_init_config_t *iotcore_init_config) {
  int32_t *clientID = NULL;
  clientID = malloc(sizeof(int32_t));
  *clientID = 0;
// Start event loops so that all events are recieved
#ifdef CONFIG_ENABLE_IOTCORE_EVENTS
  start_iotcore_event_loop(); // This will start all 3 event loops so events
                              // posted to them don't cause errors.
#ifdef CONFIG_ENABLE_SYSTEM_MONITORING
  start_system_status_reporting(clientID);
#endif
#ifdef CONFIG_ENABLE_ERROR_HANDLING
  start_iotcore_error_handler(clientID);
#endif
#endif

// Initialize NVS
#ifdef CONFIG_ENABLE_NVS
  ESP_ERROR_CHECK(nvs_read_write_init(
      NULL)); // Connectivity depends on NVS being initialized
  // This is important as it will determine if mqtt will start or not. We will
  // need to post an event for it down the line.
  readKeyValueInFlash_int32("clientId", clientID);
#if (CONFIG_ENABLE_IOTCORE_EVENTS && CONFIG_ENABLE_SYSTEM_STATUS_REPORTING)
  start_system_monitoring();
#endif
#if defined(CONFIG_ENABLE_WIFI) || defined(CONFIG_ENABLE_MODEM) ||             \
    defined(CONFIG_ENABLE_ETHERNET) || defined(CONFIG_ENABLE_EPPP_CLIENT)
  // For now this is wifi. change this to connectivity afterwards
  /**
   * Check for task fail of connectivity is done afterwards because event
   * recieving will cause mqtt to enque message but mqtt would not have been
   * initialized yet. Which requires netif handlers to be started being done
   * inside start of connectivity.
   */
  init_connectivity(iotcore_init_config != NULL
                        ? iotcore_init_config->network_priority
                        : NULL,
#ifdef ALLOW_CUSTOM_INTERNET_AVAILABILITY_CHECK
                    true
#else
                    false
#endif
  );
  init_network_event_handler(clientID);
#ifdef CONFIG_IOTCORE_MQTT_DEFAULT_CLIENT
  // If there is no clientID then dont start mqtt. it will be started when we
  // have id.
  static mqtt_credentials_t main_Global_mqtt_credentials;
  if (iotcore_init_config != NULL &&
      iotcore_init_config->mqtt_credentials != NULL) {
    memcpy(&main_Global_mqtt_credentials, iotcore_init_config->mqtt_credentials,
           sizeof(main_Global_mqtt_credentials));
  } else {
    main_Global_mqtt_credentials.client_transport =
        (char *)CONFIG_IOTCORE_MQTT_DEFAULT_CLIENT_TRANSPORT;
    main_Global_mqtt_credentials.port =
        (char *)CONFIG_IOTCORE_MQTT_DEFAULT_PORT;
    main_Global_mqtt_credentials.host =
        (char *)CONFIG_IOTCORE_MQTT_DEFAULT_HOST;
    main_Global_mqtt_credentials.username =
        (char *)CONFIG_IOTCORE_MQTT_DEFAULT_USERNAME;
    main_Global_mqtt_credentials.password =
        (char *)CONFIG_IOTCORE_MQTT_DEFAULT_PASSWORD;
  }

  mqtt_app_instance = calloc(1, sizeof(struct mqtt_app_instance_t));
  mqtt_app_instance->client_config.broker.address.port =
      atoi(main_Global_mqtt_credentials.port);
  mqtt_app_instance->client_config.broker.address.hostname =
      main_Global_mqtt_credentials.host;
  mqtt_app_instance->client_config.credentials.username =
      main_Global_mqtt_credentials.username;
  mqtt_app_instance->client_config.credentials.authentication.password =
      main_Global_mqtt_credentials.password;
  mqtt_app_instance->client_config.broker.address.transport =
      strcmp(main_Global_mqtt_credentials.client_transport, "mqtt") == 0
          ? MQTT_TRANSPORT_OVER_TCP
      : strcmp(main_Global_mqtt_credentials.client_transport, "mqtts") == 0
          ? MQTT_TRANSPORT_OVER_SSL
      : strcmp(main_Global_mqtt_credentials.client_transport, "ws") == 0
          ? MQTT_TRANSPORT_OVER_WS
          : MQTT_TRANSPORT_OVER_WSS;
  mqtt_app_instance->client_config.broker.verification
      .skip_cert_common_name_check = true;
  mqtt_app_instance->device_id = *clientID;
#ifdef ALLOW_CUSTOM_INTERNET_AVAILABILITY_CHECK
  mqtt_app_instance->onConnected = mqtt_connected_cb;
  mqtt_app_instance->onDisconnected = mqtt_disconnected_cb;
#endif
  app_mqtt_init(mqtt_app_instance);
  app_mqtt_start(mqtt_app_instance);
  mqtt_app_instance->client_config.session.disable_clean_session = false;
  register_iotcore_data_event(MQTT_DATA_PUBLISH, mqtt_pub_sub_event_handler,
                              mqtt_app_instance);
  register_iotcore_data_event(MQTT_SUBSCRIBE_TO_TOPIC,
                              mqtt_pub_sub_event_handler, mqtt_app_instance);
  register_iotcore_data_event(MQTT_UNSUBSCRIBE_FROM_TOPIC,
                              mqtt_pub_sub_event_handler, mqtt_app_instance);
  register_iotcore_data_event(MQTT_ON_CONNECT_PUBLICATION,
                              mqtt_pub_sub_event_handler, mqtt_app_instance);

  if (*clientID != 0) {
    char *deviceVersionInfo;
    asprintf(&deviceVersionInfo, "{\"fv\":\"%s\",\"hwv\":\"%s\",\"pt\":\"%s\"}",
             getDeviceInfo()->firmwareVersion, getDeviceInfo()->hardwareVersion,
             get_running_partition_label());
    char *deviceVersionInfoTopic;
    asprintf(&deviceVersionInfoTopic, "d/%ld/ota", *clientID);
    mqtt_app_instance->onConnectPublish(
        mqtt_app_instance,
        &(struct mqtt_message_t){.data = deviceVersionInfo,
                                 .data_len = strlen(deviceVersionInfo),
                                 .topic = deviceVersionInfoTopic,
                                 .qos = 2,
                                 .retain = true});
    free(deviceVersionInfo);
    free(deviceVersionInfoTopic);
    // Make iotcore default subscriptions here.
    struct {
      char *topic_substring;
      void *cb;
    } iotcore_devID_subscriptions[] = {
        {"s_ota", mqtt_s_ota_cb},
        // {"config/#",mqtt_subscriptions_callback_iotcore}, //Need to figure
        // out what this is for
        {"network/wifi/scan", mqtt_s_wifi_scan_cb},
        {"network/wifi/list", mqtt_s_wifi_list_cb},
        {"set_boot_partition", mqtt_set_boot_partition_cb},
        {"device_reset", mqtt_reset_device_cb},
    };
    for (int i = 0; i < (sizeof(iotcore_devID_subscriptions) /
                         (sizeof(char *) + sizeof(void *)));
         i++) {
      char *topic;
      asprintf(&topic, "d/%d/%s", mqtt_app_instance->device_id,
               iotcore_devID_subscriptions[i].topic_substring);
      post_mqtt_subscribe_event(topic, 0, iotcore_devID_subscriptions[i].cb);
      free(topic);
    }
    // No need for group subscriptions for now. that needs to be handled by
    // server end.
  }

#endif
#ifdef CONFIG_IOTCORE_SERVER_API_DEFAULT_CONFIG
  Http_credentials_t main_Global_http_credentials;
  if (iotcore_init_config != NULL &&
      iotcore_init_config->http_credentials != NULL) {
    memcpy(&main_Global_http_credentials, iotcore_init_config->http_credentials,
           sizeof(main_Global_http_credentials));
  } else {
    main_Global_http_credentials.device_api_username =
        (char *)CONFIG_IOTCORE_SERVER_API_DEFAULT_CONFIG_USERNAME;
    main_Global_http_credentials.device_api_password =
        (char *)CONFIG_IOTCORE_SERVER_API_DEFAULT_CONFIG_PASSWORD;
    main_Global_http_credentials.http_api_host =
        (char *)CONFIG_IOTCORE_SERVER_API_DEFAULT_CONFIG_URL;
  }
  iotcore_server_apis_task(main_Global_http_credentials, getDeviceInfo());
  #endif
  esp_event_handler_register_with(
      iotcore_app_event_loop, IOTCORE_APP_EVENTS_BASE,
      SYSTEM_IOTCORE_API_GOT_CLIENT_ID, onboarding_Complete_cb, NULL);
#endif
#endif

#ifdef CONFIG_ENABLE_APP_RTC
  xTaskCreatePinnedToCore(app_rtc, "app_rtc_task", 3048, NULL, 6, NULL, 1) ==
          pdPASS
      ? 0
      : post_task_create_failed_event(__FILE__, __LINE__,
                                      esp_get_free_heap_size());
#endif
}