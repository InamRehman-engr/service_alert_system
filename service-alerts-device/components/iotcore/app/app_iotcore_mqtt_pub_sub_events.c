#include "app_iotcore_mqtt_pub_sub_events.h"

#include "esp_types.h"
#include "iotcore_events.h"
#ifdef CONFIG_ENABLE_MQTT
#include "app_mqtt.h"
#endif
void mqtt_pub_sub_event_handler(void *event_handler_arg,
                                esp_event_base_t event_base, int32_t event_id,
                                void *event_data) {
#ifdef CONFIG_ENABLE_MQTT
  struct mqtt_app_instance_t *mqtt_app_instance =
      (struct mqtt_app_instance_t *)event_handler_arg;
  switch (event_id) {
  case MQTT_DATA_PUBLISH:
    mqtt_publish_t *publish = (mqtt_publish_t *)event_data;
    mqtt_app_instance->publish(
        mqtt_app_instance,
        &(struct mqtt_message_t){.topic = publish->topic,
                                 .qos = publish->qos,
                                 .retain = publish->retain,
                                 .data = publish->data,
                                 .data_len = publish->data_len});
    break;
  case MQTT_SUBSCRIBE_TO_TOPIC:
    mqtt_subscription_t *subscribe = (mqtt_subscription_t *)event_data;
    mqtt_app_instance->subscribe(
        mqtt_app_instance,
        &(struct subscription_t){.topic = subscribe->topic,
                                 .qos = subscribe->qos,
                                 .callback = subscribe->callback});
    break;
  case MQTT_UNSUBSCRIBE_FROM_TOPIC:
    mqtt_unsubscribe_t *unsubscribe = (mqtt_unsubscribe_t *)event_data;
    mqtt_app_instance->unsubscribe(mqtt_app_instance, unsubscribe->topic,
                                   unsubscribe->topic_len);
    break;
  case MQTT_ON_CONNECT_PUBLICATION:
    mqtt_publish_t *on_connect_publish = (mqtt_publish_t *)event_data;
    mqtt_app_instance->onConnectPublish(
        mqtt_app_instance,
        &(struct mqtt_message_t){.topic = on_connect_publish->topic,
                                 .qos = on_connect_publish->qos,
                                 .retain = on_connect_publish->retain,
                                 .data = on_connect_publish->data,
                                 .data_len = on_connect_publish->data_len});
    break;
  default:
    break;
  }
#endif
}