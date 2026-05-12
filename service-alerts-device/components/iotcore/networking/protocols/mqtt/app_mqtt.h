#ifndef __app_mqtt_h__
#define __app_mqtt_h__
#include "mqtt_client.h"

/*
Here is the list of goals with this library.
- Handle events related to connection.
- User facing functions are init, subscribe and publish
-   The library will provide a way to add messages for on connect events
-   The library will handle subscriptions on reconnect.


Handling of in flight messages can be done and messages can be stored on
persistent storage but that would bloat the library



*/
// #include <mqttEvent.h>

/*
    This part is not necessary but since we use mqtt to define state of internet
   as to whether the current wifi can provide internet or not. This will be kept
   for legacy support

    Disconnected state can be checked for time and changed over to different
   wifi if required
*/

typedef struct __attribute__((__packed__)) {
  char *port;
  char *host;
  char *username;
  char *password;
  char *client_transport;
} mqtt_credentials_t;

typedef enum mqtt_state_t {
  MQTT_STATE_CONNECTED,
  MQTT_STATE_DISCONNECTED,
  MQTT_STATE_ERROR,
  MQTT_STATE_CONNECTING,
} mqtt_state_t;

typedef struct subscription_t { // 9 bytes
  char *topic;
  void (*callback)(
      const char *topic, uint16_t topic_len, const char *message,
      uint16_t message_len); // Theese should be fast and non blocking
  int mid;
  bool subscribed;
  uint8_t qos;
} subscription_t;

typedef struct mqtt_message_t { // 12 bytes
  char *topic;
  char *data;
  uint16_t data_len;
  uint8_t qos;
  bool retain;
} mqtt_message_t;

// global configs for qos ,retain
// This is your mqtt instance keep a reference to it in your app. you can have
// more than one clients with this
typedef struct mqtt_app_instance_t {
  esp_mqtt_client_config_t
      client_config; // User only needs to do connection settings
  mqtt_state_t current_state;

  // Set this to non null value prior to calling init. if this is null at init
  // time device mac will be used If null at startup it will cause LWT to use
  // mac address
  uint16_t device_id; // Please default this to null

  /* This be the actual client*/
  esp_mqtt_client_handle_t mqttclient;
  /* this can pass you the client that you can use to subscribe or publish
   but that is not going to happen as you will have a client in
   mqtt_app_instance_t and that will be used to subscribe or publish */
  /*Do not put subscriptions in this as they will be done internally, you just
   need to call subscribe once and the internal thread will make sure that
   topics are resubscribed on connection event*/
  void (*onConnected)();    // Need to be set by user
  void (*onDisconnected)(); // Need to be set by user
  bool (*subscribe)(struct mqtt_app_instance_t *instance,
                    struct subscription_t *subscription);
  bool (*unsubscribe)(struct mqtt_app_instance_t *instance,
                      char *unsubscribe_topic, size_t len);
  int (*publish)(struct mqtt_app_instance_t *instance,
                 struct mqtt_message_t *publication);
  bool (*onConnectPublish)(struct mqtt_app_instance_t *instance,
                           struct mqtt_message_t *publication);
  uint16_t current_number_of_subscriptions;
  uint16_t current_number_of_on_connect_publications;
  char *fragmented_message;
  int fragmented_message_size;
  char *topic;
  int topic_len;
  struct subscription_t *subscriptions;
  struct mqtt_message_t *onConnectPublications;

} mqtt_app_instance_t;

void app_mqtt_init(struct mqtt_app_instance_t *mqtt_app_instance);
void app_mqtt_start(struct mqtt_app_instance_t *mqtt_app_instance);
void app_mqtt_stop(struct mqtt_app_instance_t *mqtt_app_instance);
void app_mqtt_deinit(struct mqtt_app_instance_t *mqtt_app_instance);
#endif