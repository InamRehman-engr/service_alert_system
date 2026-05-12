#ifndef __app_iotcore_h__
#define __app_iotcore_h__
#include "esp_http_server.h"
#include "esp_types.h"
#ifdef CONFIG_ENABLE_IOTCORE_SERVER_API
#include "iotcore_server_apis.h"
#endif
#ifdef CONFIG_ENABLE_MQTT
#include "app_mqtt.h"
#endif

typedef struct {
  uint8_t *network_priority;
#ifdef CONFIG_ENABLE_IOTCORE_SERVER_API
  Http_credentials_t *http_credentials;
#endif
#ifdef CONFIG_ENABLE_MQTT
  mqtt_credentials_t *mqtt_credentials;
#endif
} iotcore_init_config_t;
/**
 * This function will initialize everything needed by iotcore. basics.
 * Including but not limited to nvs, connectivity, mqtt, api_call, webserver.
 * This should be the first function to call in your application
 * Provide iotcore_init_config_t as input or NULL for default configuration
 *
 * @param iotcore_init_config
 * @return void
 */
void init_iotcore(iotcore_init_config_t *iotcore_init_config);

/**
 * Provided is a function that can be declared to enable custom handler
 * registrations for webserver.
 *
 */
void register_custom_webserver_handlers(httpd_handle_t webserver_handler);
#endif //__app_iotcore_h__