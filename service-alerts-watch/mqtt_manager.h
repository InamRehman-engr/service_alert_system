#ifndef MQTT_MANAGER_H
#define MQTT_MANAGER_H

#include <WiFi.h>
#include <PubSubClient.h>

extern WiFiClient espClient;
extern PubSubClient mqttClient;
extern int okbtn_state;
extern int last_alert_button_arr[6];
extern bool block_swipe_and_lockscreen;
extern bool alert_ads_received;

void mqtt_reconnect();
void mqtt_callback(char* topic, byte* payload, unsigned int length);

#endif