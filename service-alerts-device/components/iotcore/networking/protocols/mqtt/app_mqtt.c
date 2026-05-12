#include "app_mqtt.h"
#include "esp_log.h"
#include "iotcore_events.h"
#include "sdkconfig.h"
#include "sysinfo.h"
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#if CONFIG_MBEDTLS_CERTIFICATE_BUNDLE
#include "esp_crt_bundle.h"
#endif

static const char *TAG = "MQTT";

static char last_will_msg[] = "{\"status\":\"offline\"}";
static char online_msg[] = "{\"status\":\"online\"}";
char last_will_topic[39];

int match_topic(char *sub_topic, char *inbound_topic, int inbound_topic_len) {
  const char *sub_ptr = sub_topic;
  const char *inbound_ptr = inbound_topic;
  while (*sub_ptr && *inbound_ptr) {
    if (*sub_ptr == '+') {
      while (*inbound_ptr && *inbound_ptr != '/') {
        inbound_ptr++;
      }
      sub_ptr++;
    } else if (*sub_ptr == '#') {
      return 1;
    } else {
      if (*sub_ptr != *inbound_ptr) {
        return 0;
      }
      sub_ptr++;
      inbound_ptr++;
    }

    if (*sub_ptr == '/' && *inbound_ptr == '/') {
      sub_ptr++;
      inbound_ptr++;
    } else if (*sub_ptr == '/' || *inbound_ptr == '/') {
      return 0;
    }
  }
  if (*sub_ptr == '#' && *(sub_ptr + 1) == '\0') {
    return 1;
  }
  return *sub_ptr == '\0' && *inbound_ptr == '\0';
}

static void mqtt_event_handler(void *handler_args, esp_event_base_t base,
                               int32_t event_id, void *event_data) {
  struct mqtt_app_instance_t *instance = handler_args;
  // Handler args are custom arguments passed to the client while registration.
  // can be user defined
  ESP_LOGD(TAG,
           "Event dispatched from event loop base=%s, event_id=%" PRIi32 "",
           base, event_id);
  esp_mqtt_event_handle_t event = event_data;
  esp_mqtt_client_handle_t client = event->client;
  int i = 0;
  switch (event_id) {
  case MQTT_EVENT_ERROR:
    ESP_LOGE(TAG, "MQTT_EVENT_ERROR");
    instance->current_state = MQTT_STATE_ERROR;
    post_iotcore_app_event(MQTT_CONNECTIVITY_STATUS, &instance->current_state,
                           sizeof(instance->current_state));
    break;
  case MQTT_EVENT_CONNECTED:
    ESP_LOGI(TAG, "MQTT_EVENT_CONNECTED");
    instance->current_state = MQTT_STATE_CONNECTED;
    post_iotcore_app_event(MQTT_CONNECTIVITY_STATUS, &instance->current_state,
                           sizeof(instance->current_state));
    if (instance->onConnected != NULL) {
      instance->onConnected();
    }

    // Redo subscriptions
    i = 0;
    while (i < instance->current_number_of_subscriptions) {
      instance->subscriptions[i].mid =
          esp_mqtt_client_subscribe(client, instance->subscriptions[i].topic,
                                    instance->subscriptions[i].qos);
      ESP_LOGI(TAG, "Subscribed to topic %s with mid %d",
               instance->subscriptions[i].topic,
               instance->subscriptions[i].mid);
      i++;
    }
    // Send on connect messages
    i = 0;
    while (i < instance->current_number_of_on_connect_publications) {
      instance->publish(instance, &instance->onConnectPublications[i]);
      i++;
    }
    break;
  case MQTT_EVENT_DISCONNECTED:
    // Need to do something about in flight messages here if not being done
    // automatically
    ESP_LOGE(TAG, "MQTT_EVENT_DISCONNECTED");
    instance->current_state = MQTT_STATE_DISCONNECTED;
    post_iotcore_app_event(MQTT_CONNECTIVITY_STATUS, &instance->current_state,
                           sizeof(instance->current_state));
    if (instance->onDisconnected != NULL) {
      instance->onDisconnected();
    }
    for (int x = 0; x < instance->current_number_of_subscriptions; x++) {
      instance->subscriptions[x].subscribed = false;
    }
    break;
  case MQTT_EVENT_SUBSCRIBED:
    // Update status of subscription
    // While instead of for for not running if no subscriptions
    i = 0;
    while (i < instance->current_number_of_subscriptions) {
      if (event->msg_id == instance->subscriptions[i].mid) {
        ESP_LOGI(TAG, "Subscription Successful to topic %s",
                 instance->subscriptions[i].topic);
        instance->subscriptions[i].subscribed = true;
        break;
      }
      i++;
    }
    break;
  case MQTT_EVENT_UNSUBSCRIBED:
    // Update status of subscription
    // While instead of for for not running if no subscriptions
    ESP_LOGI(TAG, "Unsubscribed from topic");
    break;
  case MQTT_EVENT_PUBLISHED:
    ESP_LOGI(TAG, "Message Published to topic");
    break;
  case MQTT_EVENT_DATA:
    ESP_LOGI(TAG, "MQTT Data event.");
    char *complete_data = NULL;
    if (event->total_data_len != event->data_len) {
      ESP_LOGW(TAG, "Data Chunked");
      if (instance->fragmented_message == NULL) {
        instance->fragmented_message = (char *)malloc(event->total_data_len);
        instance->topic = (char *)malloc(event->topic_len + 1);
        if (!instance->fragmented_message || !instance->topic) {
          ESP_LOGE(TAG, "Failed to allocate memory");
          return;
        }
        memcpy(instance->topic, event->topic, event->topic_len);
        instance->topic_len = event->topic_len;
      }
      memcpy(&instance->fragmented_message[instance->fragmented_message_size],
             event->data, event->data_len);
      instance->fragmented_message_size += event->data_len;
      if (instance->fragmented_message_size < event->total_data_len) {
        return;
      }
      complete_data = malloc(event->total_data_len);
      memcpy(complete_data, instance->fragmented_message,
             event->total_data_len);
    } else {
      complete_data = malloc(event->total_data_len);
      memcpy(complete_data, event->data, event->total_data_len);
      instance->topic = event->topic;
      instance->topic_len = event->topic_len;
    }
    ESP_LOGI(TAG, "Data Complete");
    instance->topic[instance->topic_len] = '\0';
    for (int i = 0; i < instance->current_number_of_subscriptions; i++) {
      if (match_topic(instance->subscriptions[i].topic, instance->topic,
                      instance->topic_len) == 1) {
        if (instance->subscriptions[i].subscribed &&
            instance->subscriptions[i].callback) {
          instance->subscriptions[i].callback(
              instance->topic, instance->topic_len, complete_data,
              event->total_data_len);
        }
      }
    }
    if (instance->fragmented_message_size) {
      free(instance->fragmented_message);
      instance->fragmented_message = NULL;
      instance->fragmented_message_size = 0;
      free(instance->topic);
      instance->topic = NULL;
      instance->topic_len = 0;
    }
    free(complete_data);
    complete_data = NULL;

    break;
  case MQTT_EVENT_BEFORE_CONNECT:
    // Connection started. probably helpful for wifi
    instance->current_state = MQTT_STATE_CONNECTING;
    post_iotcore_app_event(MQTT_CONNECTIVITY_STATUS, &instance->current_state,
                           sizeof(instance->current_state));
    ESP_LOGI(TAG, "MQTT_EVENT_BEFORE_CONNECT");
    break;
  case MQTT_EVENT_DELETED:
    break;
#if (ESP_IDF_VERSION_MAJOR >= 5 && ESP_IDF_VERSION_MINOR > 0)
  case MQTT_USER_EVENT:
    break;
#endif
  default:
    break;
  }
}

void mqtt_configuration_set(esp_mqtt_client_config_t *client_config) {
  // Some settings will be made so that they are global and common across
  // multiple clients

  // Connection Settings
  // Broker address

  // Verification. This is for certificate
#ifdef CONFIG_MBEDTLS_CERTIFICATE_BUNDLE
  client_config->broker.verification.crt_bundle_attach =
      esp_crt_bundle_attach; // Will be used when using ws or wss
#endif
  // Some of our servers require CA name check to be skipped. User can skip that
  // externally if required Credentials

  // Network
  client_config->network.reconnect_timeout_ms = CONFIG_MQTT_RECONNECT_TIMEOUT;
  client_config->network.refresh_connection_after_ms =
      CONFIG_MQTT_REFRESH_CONNECTION_MS;
  // Session Settings
  // Last Will
  client_config->session.last_will.msg = last_will_msg;
  client_config->session.last_will.msg_len = strlen(last_will_msg);
  client_config->session.last_will.topic = last_will_topic;
  client_config->session.last_will.qos = 1;
  client_config->session.last_will.retain = true;

  // Keepalive
  client_config->session.keepalive = CONFIG_MQTT_KEEPALIVE_INTERVAL;
  client_config->session.message_retransmit_timeout =
      CONFIG_MQTT_RETRANSMIT_TIMEOUT;

  // Task
  client_config->task.priority = CONFIG_MQTT_TASK_PRIORITY;
  client_config->task.stack_size = CONFIG_MQTT_TASK_STACK_SIZE;

  // Buffer. Message Buffer. Define this to be so that your longest message can
  // fit in this buffer during sending and recieving oterwise sectioned messages
  // might be transmitted or recieved
  client_config->buffer.size = CONFIG_MQTT_BUFFER_SIZE;
}

bool unsubscribe(struct mqtt_app_instance_t *instance, char *unsubscribe_topic,
                 size_t len) {
  if (unsubscribe_topic != NULL && unsubscribe_topic[0] != '\0') {
    for (int i = 0; i < instance->current_number_of_subscriptions; i++) {
      if (strncmp(unsubscribe_topic, instance->subscriptions[i].topic, len) ==
          0) {
        ESP_LOGI(TAG, "unsubscribing from topic : %s.",
                 instance->subscriptions[i].topic);
        esp_mqtt_client_unsubscribe(instance->mqttclient,
                                    instance->subscriptions[i].topic);
        subscription_t *newList =
            malloc(sizeof(struct subscription_t) *
                   (instance->current_number_of_subscriptions - 1));
        memcpy(newList, instance->subscriptions, i * sizeof(subscription_t));
        memcpy(&newList[i], &instance->subscriptions[i + 1],
               (instance->current_number_of_subscriptions - (i + 1)) *
                   sizeof(subscription_t));
        void *temp = instance->subscriptions;
        instance->subscriptions = newList;
        instance->current_number_of_subscriptions--;
        free(temp);
        return true;
      }
    }
  } else {
    ESP_LOGE(TAG, "unsub topic not initalized.");
    return false;
  }
  return false;
}

/*
    Topic string must be null terminated
*/
bool subscribe(struct mqtt_app_instance_t *instance,
               struct subscription_t *subscription) {
  int i = 0;
  while (i < instance->current_number_of_subscriptions) {
    if (instance->subscriptions[i].topic[0] != '\0' &&
        strcmp(instance->subscriptions[i].topic, subscription->topic) == 0) {
      return true;
    }
    i++;
  }

  void *priv_pointer = instance->subscriptions;
  instance->subscriptions =
      realloc(instance->subscriptions,
              sizeof(struct subscription_t) *
                  (instance->current_number_of_subscriptions + 1));
  if (instance->subscriptions == NULL) {
    instance->subscriptions = priv_pointer;
    return false;
  }

  struct subscription_t *newSubscription =
      &instance->subscriptions[instance->current_number_of_subscriptions];
  newSubscription->topic = (char *)malloc(strlen(subscription->topic) + 1);
  if (newSubscription->topic == NULL) {
    instance->subscriptions = priv_pointer;
    return false;
  }
  strcpy(newSubscription->topic, subscription->topic);

  newSubscription->qos = subscription->qos;
  newSubscription->callback = subscription->callback;
  instance->current_number_of_subscriptions++;
  if (instance->current_state == MQTT_STATE_CONNECTED) {
    newSubscription->mid = esp_mqtt_client_subscribe(
        instance->mqttclient, newSubscription->topic, newSubscription->qos);
    ESP_LOGI(TAG, "Subscribed to topic %s with mid %d", newSubscription->topic,
             newSubscription->mid);
  }
  return true;
}

/**
 * We will gaurantee delivery of qos>0 messages. qos==0 messages may be lost
 * depending on internet connectivity
 */
int publish(struct mqtt_app_instance_t *instance,
            struct mqtt_message_t *publication) {
  return publication->qos == 0
             ? esp_mqtt_client_publish(instance->mqttclient, publication->topic,
                                       publication->data, publication->data_len,
                                       publication->qos, publication->retain)
             : esp_mqtt_client_enqueue(instance->mqttclient, publication->topic,
                                       publication->data, publication->data_len,
                                       publication->qos, publication->retain,
                                       false);
}

/*
    Careful with this. not checking whether already in list
*/
bool onConnectPublish(struct mqtt_app_instance_t *instance,
                      struct mqtt_message_t *publication) {
  // Check if message on same topic exists
  for (int i = 0; i < instance->current_number_of_on_connect_publications;
       i++) {
    if (strcmp(instance->onConnectPublications[i].topic, publication->topic) ==
        0) {
      free(instance->onConnectPublications[i].data);
      instance->onConnectPublications[i].data = malloc(publication->data_len);
      memcpy(instance->onConnectPublications[i].data, publication->data,
             publication->data_len);
      instance->onConnectPublications[i].data_len = publication->data_len;
      instance->onConnectPublications[i].qos = publication->qos;
      instance->onConnectPublications[i].retain = publication->retain;
      return true;
    }
  }
  struct mqtt_message_t *newArray =
      realloc(instance->onConnectPublications,
              sizeof(struct mqtt_message_t) *
                  (instance->current_number_of_on_connect_publications + 1));
  if (newArray == NULL) {
    return false;
  }
  instance->onConnectPublications = newArray;

  struct mqtt_message_t *newEntry =
      &instance->onConnectPublications
           [instance->current_number_of_on_connect_publications];
  newEntry->topic = strdup(publication->topic);
  newEntry->data = malloc(publication->data_len);
  memcpy(newEntry->data, publication->data, publication->data_len);
  newEntry->data_len = publication->data_len;
  newEntry->qos = publication->qos;
  newEntry->retain = publication->retain;
  instance->current_number_of_on_connect_publications++;
  return true;
}

void app_mqtt_init(struct mqtt_app_instance_t *mqtt_app_instance) {
  // This part for setting of the device id
  mqtt_app_instance->current_number_of_subscriptions = 0;
  mqtt_app_instance->subscribe = subscribe;
  mqtt_app_instance->unsubscribe = unsubscribe;
  mqtt_app_instance->publish = publish;
  mqtt_app_instance->onConnectPublish = onConnectPublish;
  mqtt_app_instance->fragmented_message = NULL;
  // mqtt_app_instance->client_config
  char *_deviceId = NULL; // This will persist among brokers
  device_info_t *deviceInfo = getDeviceInfo();
#if (CONFIG_MQTT_USE_MAC_ADDRESS_AS_DEVICE_ID == true)
  _deviceId = deviceInfo->registrationNumber;
#else
  if (!mqtt_app_instance->device_id) {
    _deviceId = deviceInfo->registrationNumber;
  } else {
    asprintf(&_deviceId, "%d", mqtt_app_instance->device_id);
  }
#endif

  sprintf(last_will_topic, "d/%s/status", _deviceId);
  mqtt_app_instance->onConnectPublish(
      mqtt_app_instance,
      &(struct mqtt_message_t){.topic = last_will_topic,
                               .data = online_msg,
                               .data_len = strlen(online_msg),
                               .qos = 0,
                               .retain = true});

  mqtt_configuration_set(&mqtt_app_instance->client_config);
  mqtt_app_instance->mqttclient =
      esp_mqtt_client_init(&(mqtt_app_instance->client_config));
  esp_mqtt_client_register_event(mqtt_app_instance->mqttclient,
                                 ESP_EVENT_ANY_ID, mqtt_event_handler,
                                 (void *)mqtt_app_instance);
}

void app_mqtt_start(struct mqtt_app_instance_t *mqtt_app_instance) {
  esp_mqtt_client_start(mqtt_app_instance->mqttclient);
}
void app_mqtt_stop(struct mqtt_app_instance_t *mqtt_app_instance) {

  // clear lists
  esp_mqtt_client_stop(mqtt_app_instance->mqttclient);
}

void app_mqtt_deinit(struct mqtt_app_instance_t *mqtt_app_instance) {
  esp_mqtt_client_destroy(mqtt_app_instance->mqttclient);
}
