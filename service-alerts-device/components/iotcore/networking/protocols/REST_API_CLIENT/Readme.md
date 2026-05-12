# Rest Api Client

This module is a wrapper to the esp's internally available http library and provides a user friendly way of handling of data. This is also used by nttps_ota_hanndler to provide resume functionality for ota.

This is how you would use the internal http library if you wanted to use it.

```
    esp_http_client_config_t config = {
      .url = url,
      .method = HTTP_METHOD_GET,
      .event_handler = handler,
#ifdef CONFIG_MBEDTLS_CERTIFICATE_BUNDLE
      .crt_bundle_attach = esp_crt_bundle_attach,
#endif
      .user_agent = CONFIG_HTTP_REST_CLIENT_USER_AGENT,
      .user_data = http_rest_recv_buffer,
      .buffer_size_tx = calculate_tx_buffer_size(url, NULL, custom_headers, num_headers),
  };

  client = esp_http_client_init(&config);
  if ((custom_headers == NULL ? 0 : num_headers)>0){
    for (size_t i = 0; i < num_headers; i++) {
        esp_http_client_set_header(client, custom_headers[i].key, custom_headers[i].value);
    }
  }
  else {
    esp_http_client_set_header(client, "Content-Type", "application/json");
  }

  ret = esp_http_client_perform(client);

  ESP_LOGD(TAG, "Get request complete");

  if (ESP_OK != ret)
  {
    ESP_LOGE(TAG, "HTTP GET request failed: %s", esp_err_to_name(ret));
    esp_http_client_cleanup(client);
    return ret;
  }

  int status_code = esp_http_client_get_status_code(client);

  http_rest_recv_buffer->status_code = status_code;

  ESP_LOGD(TAG, "Cleaning up client before returning");
  esp_http_client_cleanup(client);
```


This is the usage through the use of library
```
    esp_err_t err = http_rest_client_post(host, post_data, strlen(post_data), &response_buffer, NULL, 0);
```
This drastically reduces the number of things that are needed to be understood by the developer to use http requests.


## Special handling for https requests

Https requests require that the server identity be verified before processing the actual request. mbedtls is used for this purpose. The identity is verified using the provided CA certificate.

### What this mean in terms of usage

This just means that user will either need to use a certificate bundle or provide a CA certificate in case of custom server.


Certificate handling for server verification will be done through menuconfig

This config requires a folder in application root containing server certificates.
python C:/Espressif/frameworks/esp-idf-v5.1/components/mbedtls/esp_crt_bundle/gen_crt_bundle.py --input D:/Cowlar_Repos/solar-esp32/server_certs

this command modified with your paths will allow you to generate a bundle and test if the certificates inside your folder are valid or not.



Here is config that needs to be followed in production environment strictly.
```
#########################PROD CONFIG########################
#
# Certificate Bundle
#
CONFIG_MBEDTLS_CERTIFICATE_BUNDLE=y
# CONFIG_MBEDTLS_CERTIFICATE_BUNDLE_DEFAULT_FULL is not set
# CONFIG_MBEDTLS_CERTIFICATE_BUNDLE_DEFAULT_CMN is not set
CONFIG_MBEDTLS_CERTIFICATE_BUNDLE_DEFAULT_NONE=y
CONFIG_MBEDTLS_CUSTOM_CERTIFICATE_BUNDLE=y
CONFIG_MBEDTLS_CUSTOM_CERTIFICATE_BUNDLE_PATH="server_certs"
CONFIG_MBEDTLS_CERTIFICATE_BUNDLE_MAX_CERTS=200
# end of Certificate Bundle
``````
The production environment will have no more than the required number of certificates. Use this as binding as more certificates equals to more memory size

Dev environment can work with these 2 configs. these allow you to 

1st config is this one. This is more recommended for dev as this allows secure connections just by embedding full list of CAs in the firmware
```
#
# Certificate Bundle
#
CONFIG_MBEDTLS_CERTIFICATE_BUNDLE=y
CONFIG_MBEDTLS_CERTIFICATE_BUNDLE_DEFAULT_FULL=y
# CONFIG_MBEDTLS_CERTIFICATE_BUNDLE_DEFAULT_CMN is not set
# CONFIG_MBEDTLS_CERTIFICATE_BUNDLE_DEFAULT_NONE is not set
# CONFIG_MBEDTLS_CUSTOM_CERTIFICATE_BUNDLE is not set
CONFIG_MBEDTLS_CERTIFICATE_BUNDLE_MAX_CERTS=200
# end of Certificate Bundle
```
2nd config is this one. This option is not recommended for dev as it allows insecure connections. Should only be used just for testing.
```
#
# Certificate Bundle
#
# CONFIG_MBEDTLS_CERTIFICATE_BUNDLE is not set
# end of Certificate Bundle

#
# ESP-TLS
#
CONFIG_ESP_TLS_USING_MBEDTLS=y
# CONFIG_ESP_TLS_USE_SECURE_ELEMENT is not set
# CONFIG_ESP_TLS_CLIENT_SESSION_TICKETS is not set
# CONFIG_ESP_TLS_SERVER is not set
# CONFIG_ESP_TLS_PSK_VERIFICATION is not set
CONFIG_ESP_TLS_INSECURE=y
CONFIG_ESP_TLS_SKIP_SERVER_CERT_VERIFY=y
# end of ESP-TLS
```