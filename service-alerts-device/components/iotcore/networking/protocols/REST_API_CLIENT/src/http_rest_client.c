
#include "http_rest_client.h"

static const char *TAG = "http_rest_client";
SemaphoreHandle_t httpsemphr = NULL;

#ifdef CONFIG_HTTP_REST_CLIENT_USE_LOCK
#define HTTP_REST_API_LOCK()                                                   \
  do {                                                                         \
    if (httpsemphr == NULL) {                                                  \
      httpsemphr = xSemaphoreCreateMutex();                                    \
      if (httpsemphr == NULL) {                                                \
        ESP_LOGE(TAG, "Semaphore creation failed");                            \
      }                                                                        \
    }                                                                          \
    if (httpsemphr != NULL) {                                                  \
      if (xSemaphoreTake(httpsemphr, portMAX_DELAY) != pdTRUE) {               \
        ESP_LOGE(TAG, "Failed to take semaphore");                             \
        return ESP_FAIL;                                                       \
      }                                                                        \
    }                                                                          \
  } while (0)

#define HTTP_REST_API_UNLOCK()                                                 \
  do {                                                                         \
    xSemaphoreGive(httpsemphr);                                                \
  } while (0)
#else
#define HTTP_REST_API_LOCK()
#define HTTP_REST_API_UNLOCK()
#endif

size_t calculate_tx_buffer_size(char *url, const char *body_data,
                                http_header_t *custom_headers,
                                size_t num_headers) {
  // (url + CRLF + "-b " + CRLF)
  size_t body_size = strlen(url) + (!body_data ? 0 : strlen(body_data)) + 11;

  size_t headers_size = 0;
  for (size_t i = 0; i < (custom_headers == NULL ? 0 : num_headers); i++) {
    // Calculate the size of each header ("-H " + key + value + ": " + CRLF)
    headers_size +=
        strlen(custom_headers[i].key) + strlen(custom_headers[i].value) + 7;
  }

  /// ESPs http client is dumb in a number of ways. it knows exactly how much
  /// memory it needs for the request but rather than reallocating the buffer.
  /// it just fails if there is not enough memory It expects from the user to
  /// provide a tx buffer length when it is adding data to that request without
  /// asking. To circumvent the issue of tx length overflowing and headers not
  /// getting fit in the provided length These are all the potential data that
  /// the esp can add to the request. This is how much reserve memory should be
  /// provided to each tx buffer
  // Content-Type: application/json \n
  // headers_size += 33;
  // User-Agent: IDF HTTP REST Client/1.0 \n
  // headers_size += 39;
  // Content-Length: body_size \n  or Transfer-Encoding: chunked \n
  // headers_size += 29;
  // Host: 100 length \n
  // headers_size += 110;
  headers_size += 211;
  // Calculate the total buffer size (body + headers)
  size_t total_size = body_size + headers_size;
  return total_size;
}

esp_err_t http_rest_client_get(char *url,
                               http_rest_recv_buffer_t *http_rest_recv_buffer,
                               http_header_t *custom_headers,
                               size_t num_headers) {
  return http_rest_client_get_custom_handler(url, http_rest_recv_buffer,
                                             http_event_handler, custom_headers,
                                             num_headers);
}

esp_err_t http_rest_client_get_custom_handler(
    char *url, http_rest_recv_buffer_t *http_rest_recv_buffer,
    http_event_handle_cb handler, http_header_t *custom_headers,
    size_t num_headers) {
  esp_err_t ret = ESP_OK;
  HTTP_REST_API_LOCK();
  esp_http_client_handle_t client;

  // Zero out the buffer for safety
  memset(http_rest_recv_buffer, 0, sizeof(http_rest_recv_buffer_t));

  ESP_LOGD(TAG, "Initializing client");

  esp_http_client_config_t config = {
      .url = url,
      .method = HTTP_METHOD_GET,
      .event_handler = handler,
#ifdef CONFIG_MBEDTLS_CERTIFICATE_BUNDLE
      .crt_bundle_attach = esp_crt_bundle_attach,
#endif
      .user_agent = CONFIG_HTTP_REST_CLIENT_USER_AGENT,
      .user_data = http_rest_recv_buffer,
      .buffer_size_tx =
          calculate_tx_buffer_size(url, NULL, custom_headers, num_headers),
      .buffer_size = CONFIG_HTTP_REST_CLIENT_RECEIVE_BUFFER_SIZE,
  };

  client = esp_http_client_init(&config);
  if ((custom_headers == NULL ? 0 : num_headers) > 0) {
    for (size_t i = 0; i < num_headers; i++) {
      esp_http_client_set_header(client, custom_headers[i].key,
                                 custom_headers[i].value);
    }
  } else {
    esp_http_client_set_header(client, "Content-Type", "application/json");
  }

  ret = esp_http_client_perform(client);

  ESP_LOGD(TAG, "Get request complete");

  if (ESP_OK != ret) {
    ESP_LOGE(TAG, "HTTP GET request failed: %s", esp_err_to_name(ret));
    esp_http_client_cleanup(client);
    HTTP_REST_API_UNLOCK();
    return ret;
  }

  int status_code = esp_http_client_get_status_code(client);

  http_rest_recv_buffer->status_code = status_code;

  ESP_LOGD(TAG, "Cleaning up client before returning");
  esp_http_client_cleanup(client);
  HTTP_REST_API_UNLOCK();
  return ret;
}

esp_err_t
http_rest_client_delete(char *url,
                        http_rest_recv_buffer_t *http_rest_recv_buffer,
                        http_header_t *custom_headers, size_t num_headers) {

  esp_err_t ret = ESP_OK;
  HTTP_REST_API_LOCK();
  esp_http_client_handle_t client;

  // Zero out the buffer for safety
  memset(http_rest_recv_buffer, 0, sizeof(http_rest_recv_buffer_t));

  ESP_LOGD(TAG, "Initializing client");

  esp_http_client_config_t config = {
      .url = url,
      .method = HTTP_METHOD_DELETE,
      .event_handler = http_event_handler,
#ifdef CONFIG_MBEDTLS_CERTIFICATE_BUNDLE
      .crt_bundle_attach = esp_crt_bundle_attach,
#endif
      .user_agent = CONFIG_HTTP_REST_CLIENT_USER_AGENT,
      .user_data = http_rest_recv_buffer,
      .buffer_size_tx =
          calculate_tx_buffer_size(url, NULL, custom_headers, num_headers),
      .buffer_size = CONFIG_HTTP_REST_CLIENT_RECEIVE_BUFFER_SIZE,
  };

  client = esp_http_client_init(&config);

  if ((custom_headers == NULL ? 0 : num_headers) > 0) {
    for (size_t i = 0; i < num_headers; i++) {
      esp_http_client_set_header(client, custom_headers[i].key,
                                 custom_headers[i].value);
    }
  } else {
    esp_http_client_set_header(client, "Content-Type", "application/json");
  }

  ret = esp_http_client_perform(client);

  ESP_LOGD(TAG, "Get request complete");

  if (ESP_OK != ret) {
    ESP_LOGE(TAG, "HTTP GET request failed: %s", esp_err_to_name(ret));
    esp_http_client_cleanup(client);
    HTTP_REST_API_UNLOCK();
    return ret;
  }

  int status_code = esp_http_client_get_status_code(client);

  http_rest_recv_buffer->status_code = status_code;

  ESP_LOGD(TAG, "Cleaning up client before returning");
  esp_http_client_cleanup(client);
  HTTP_REST_API_UNLOCK();
  return ret;
}

esp_err_t http_rest_client_post(char *url, char *body_data, uint16_t data_size,
                                http_rest_recv_buffer_t *http_rest_recv_buffer,
                                http_header_t *custom_headers,
                                size_t num_headers) {
  esp_err_t ret = ESP_OK;
  HTTP_REST_API_LOCK();
  esp_http_client_handle_t client;

  // Zero out the buffer for safety
  memset(http_rest_recv_buffer, 0, sizeof(http_rest_recv_buffer_t));

  ESP_LOGD(TAG, "Initializing client");

  esp_http_client_config_t config = {
      .url = url,
      .method = HTTP_METHOD_POST,
      .event_handler = http_event_handler,
#ifdef CONFIG_MBEDTLS_CERTIFICATE_BUNDLE
      .crt_bundle_attach = esp_crt_bundle_attach,
#endif
      .user_agent = CONFIG_HTTP_REST_CLIENT_USER_AGENT,
      .user_data = http_rest_recv_buffer,
      .buffer_size_tx =
          calculate_tx_buffer_size(url, body_data, custom_headers, num_headers),
      .buffer_size = CONFIG_HTTP_REST_CLIENT_RECEIVE_BUFFER_SIZE,
  };

  client = esp_http_client_init(&config);
  if ((custom_headers == NULL ? 0 : num_headers) > 0) {
    for (size_t i = 0; i < num_headers; i++) {
      esp_http_client_set_header(client, custom_headers[i].key,
                                 custom_headers[i].value);
    }
  } else {
    esp_http_client_set_header(client, "Content-Type", "application/json");
  }
  esp_http_client_set_post_field(client, body_data, data_size);
  ret = esp_http_client_perform(client);

  ESP_LOGD(TAG, "Get request complete");

  if (ESP_OK != ret) {
    ESP_LOGE(TAG, "HTTP GET request failed: %s", esp_err_to_name(ret));
    esp_http_client_cleanup(client);
    HTTP_REST_API_UNLOCK();
    return ret;
  }

  int status_code = esp_http_client_get_status_code(client);

  http_rest_recv_buffer->status_code = status_code;

  ESP_LOGD(TAG, "Cleaning up client before returning");
  esp_http_client_cleanup(client);
  HTTP_REST_API_UNLOCK();
  return ret;
}

esp_err_t http_rest_client_put(char *url, char *body_data, uint16_t data_size,
                               http_rest_recv_buffer_t *http_rest_recv_buffer,
                               http_header_t *custom_headers,
                               size_t num_headers) {
  esp_err_t ret = ESP_OK;
  HTTP_REST_API_LOCK();
  esp_http_client_handle_t client;

  // Zero out the buffer for safety
  memset(http_rest_recv_buffer, 0, sizeof(http_rest_recv_buffer_t));

  ESP_LOGD(TAG, "Initializing client");

  esp_http_client_config_t config = {
      .url = url,
      .method = HTTP_METHOD_PUT,
      .event_handler = http_event_handler,
#ifdef CONFIG_MBEDTLS_CERTIFICATE_BUNDLE
      .crt_bundle_attach = esp_crt_bundle_attach,
#endif
      .user_agent = CONFIG_HTTP_REST_CLIENT_USER_AGENT,
      .user_data = http_rest_recv_buffer,
      .buffer_size_tx =
          calculate_tx_buffer_size(url, body_data, custom_headers, num_headers),
      .buffer_size = CONFIG_HTTP_REST_CLIENT_RECEIVE_BUFFER_SIZE,
  };

  client = esp_http_client_init(&config);

  if ((custom_headers == NULL ? 0 : num_headers) > 0) {
    for (size_t i = 0; i < num_headers; i++) {
      esp_http_client_set_header(client, custom_headers[i].key,
                                 custom_headers[i].value);
    }
  } else {
    esp_http_client_set_header(client, "Content-Type", "application/json");
  }

  esp_http_client_set_post_field(client, body_data, data_size);

  ret = esp_http_client_perform(client);

  ESP_LOGD(TAG, "Get request complete");

  if (ESP_OK != ret) {
    ESP_LOGE(TAG, "HTTP GET request failed: %s", esp_err_to_name(ret));
    esp_http_client_cleanup(client);
    HTTP_REST_API_UNLOCK();
    return ret;
  }

  int status_code = esp_http_client_get_status_code(client);

  http_rest_recv_buffer->status_code = status_code;

  ESP_LOGD(TAG, "Cleaning up client before returning");
  esp_http_client_cleanup(client);
  HTTP_REST_API_UNLOCK();
  return ret;
}

void http_rest_client_cleanup(http_rest_recv_buffer_t *http_rest_recv_buffer) {
  if (http_rest_recv_buffer->buffer != NULL) {
    free(http_rest_recv_buffer->buffer);
  }
  ESP_LOGD(TAG, "Cleaned up http_rest_recv_buffer");
}
