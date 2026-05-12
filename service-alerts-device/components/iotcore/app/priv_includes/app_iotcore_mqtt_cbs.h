#ifndef __app_iotcore_mqtt_cbs__
#define __app_iotcore_mqtt_cbs__

#include "esp_types.h"

void mqtt_s_ota_cb(char *topic, size_t topiclen, char *data, size_t datalen);
void mqtt_s_wifi_scan_cb(char *topic, size_t topiclen, char *data,
                         size_t datalen);
void mqtt_s_wifi_list_cb(char *topic, size_t topiclen, char *data,
                         size_t datalen);
void mqtt_set_boot_partition_cb(char *topic, size_t topiclen, char *data,
                                size_t datalen);
void mqtt_reset_device_cb(char *topic, size_t topiclen, char *data,
                          size_t datalen);
#endif /* defined(__app_iotcore_mqtt_cbs__) */