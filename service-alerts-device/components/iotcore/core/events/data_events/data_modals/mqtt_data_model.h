#ifndef _mqtt_info_h_
#define _mqtt_info_h_
#include "esp_types.h"

// This will be freed as soon as data is returned from event handler. so large
// size is fine. life will be less. will live on heap
typedef struct mqtt_publish {
  char topic[100];
  char data[1000];
  size_t data_len;
  size_t topic_len;
  bool retain;
  uint8_t qos;
} mqtt_publish_t;

typedef struct mqtt_unsubscribe {
  char topic[100];
  size_t topic_len;
} mqtt_unsubscribe_t;

typedef struct subscription {
  char topic[100];
  int qos;
  void (*callback)(
      const char *topic, uint16_t topic_len, const char *message,
      uint16_t message_len); // Forward compatibility with new mqtt client
} mqtt_subscription_t;

// This needs some functions for the sake of reusability
void post_mqtt_publish_event(char *data, size_t data_len, char *topic,
                             bool retain, uint8_t qos);
void post_mqtt_on_connect_publish_event(char *data, size_t data_len,
                                        char *topic, bool retain, uint8_t qos);
void post_mqtt_subscribe_event(char *topic, int qos,
                               void (*callback)(const char *topic,
                                                uint16_t topic_len,
                                                const char *message,
                                                uint16_t message_len));
void post_mqtt_unsubscribe_event(char *unsub_topic, int len);
#endif