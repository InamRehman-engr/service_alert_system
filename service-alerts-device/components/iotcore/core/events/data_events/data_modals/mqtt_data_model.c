#include "mqtt_data_model.h"
#include "iotcore_data_events.h"
#include <string.h> // Needed for memcpy def
void post_mqtt_publish_event(char *data, size_t data_len, char *topic,
                             bool retain, uint8_t qos) {
  mqtt_publish_t post_data = {
      .data_len = data_len,
      .qos = qos,
      .retain = retain,
  };
  memcpy(post_data.data, data, data_len);
  memcpy(post_data.topic, topic, strlen(topic));
  post_iotcore_data_event(MQTT_DATA_PUBLISH, (void *)&post_data,
                          sizeof(mqtt_publish_t));
}
void post_mqtt_on_connect_publish_event(char *data, size_t data_len,
                                        char *topic, bool retain, uint8_t qos) {
  mqtt_publish_t post_data = {
      .data_len = data_len,
      .qos = qos,
      .retain = retain,
  };
  memcpy(post_data.data, data, data_len);
  memcpy(post_data.topic, topic, strlen(topic));
  post_iotcore_data_event(MQTT_ON_CONNECT_PUBLICATION, (void *)&post_data,
                          sizeof(mqtt_publish_t));
}
void post_mqtt_subscribe_event(char *topic, int qos,
                               void (*callback)(const char *topic,
                                                uint16_t topic_len,
                                                const char *message,
                                                uint16_t message_len)) {
  mqtt_subscription_t post_data = {.callback = callback};
  memcpy(post_data.topic, topic, strlen(topic));
  post_iotcore_data_event(MQTT_SUBSCRIBE_TO_TOPIC, (void *)&post_data,
                          sizeof(mqtt_subscription_t));
}
void post_mqtt_unsubscribe_event(char *unsub_topic, int len) {
  mqtt_unsubscribe_t post_data = {
      .topic_len = len,
  };
  strncpy(post_data.topic, unsub_topic, len);
  post_iotcore_data_event(MQTT_UNSUBSCRIBE_FROM_TOPIC, (void *)&post_data,
                          sizeof(mqtt_unsubscribe_t));
}