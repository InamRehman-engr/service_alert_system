
#pragma once

#include "esp_http_client.h"
#include "esp_log.h"
#include "esp_tls.h"
#include "http_event_handler.h"
#include "http_rest_types.h"
#if CONFIG_MBEDTLS_CERTIFICATE_BUNDLE
#include "esp_crt_bundle.h"
#endif

/**
 * @brief Get request processor.
 *
 * @param url "uri" needs to be URI including https:// or http:// otherwise will
 * crash
 * @param http_rest_recv_buffer Response buffer which will contain status code
 * and the response data if available
 * @param custom_headers User can supply custom headers if required. this is
 * useful if the data provided is form-encoded
 * @param num_headers
 * @return esp_err_t
 */
esp_err_t http_rest_client_get(char *url,
                               http_rest_recv_buffer_t *http_rest_recv_buffer,
                               http_header_t *custom_headers,
                               size_t num_headers);

/**
 * @brief http_rest_client_get but with delete
 *
 * @param url
 * @param http_rest_recv_buffer
 * @param custom_headers
 * @param num_headers
 * @return esp_err_t
 */
esp_err_t
http_rest_client_delete(char *url,
                        http_rest_recv_buffer_t *http_rest_recv_buffer,
                        http_header_t *custom_headers, size_t num_headers);

/**
 * @brief Post data request. This one is the same as get but allows putting body
 * data as an argument
 *
 * @param url
 * @param body_data
 * @param data_size
 * @param http_rest_recv_buffer
 * @param custom_headers
 * @param num_headers
 * @return esp_err_t
 */
esp_err_t http_rest_client_post(char *url, char *body_data, uint16_t data_size,
                                http_rest_recv_buffer_t *http_rest_recv_buffer,
                                http_header_t *custom_headers,
                                size_t num_headers);

/**
 * @brief Put request
 *
 * @param url
 * @param body_data
 * @param data_size
 * @param http_rest_recv_buffer
 * @param custom_headers
 * @param num_headers
 * @return esp_err_t
 */
esp_err_t http_rest_client_put(char *url, char *body_data, uint16_t data_size,
                               http_rest_recv_buffer_t *http_rest_recv_buffer,
                               http_header_t *custom_headers,
                               size_t num_headers);

/**
 * @brief Get request with custom handler. This one is the same as get but
 * allows putting custom handler as an argument. Used where response data is
 * known to be large and cannot fit inside runtime memory
 *
 * @param url
 * @param http_rest_recv_buffer
 * @param handler
 * @param custom_headers
 * @param num_headers
 * @return esp_err_t
 */
esp_err_t http_rest_client_get_custom_handler(
    char *url, http_rest_recv_buffer_t *http_rest_recv_buffer,
    http_event_handle_cb handler, http_header_t *custom_headers,
    size_t num_headers);

/**
 * @brief Cleanup function. Cleans the memory assigned by the event handler
 *
 * @param http_rest_recv_buffer
 */
void http_rest_client_cleanup(http_rest_recv_buffer_t *http_rest_recv_buffer);
/// TODO: Add HTTP2 support. Currently not required.