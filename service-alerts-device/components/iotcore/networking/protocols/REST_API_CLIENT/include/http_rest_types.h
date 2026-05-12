
#pragma once

#include "cJSON.h"

/**
 * @brief Response of http_rest_client_*
 *
 */
typedef struct http_rest_recv_buffer_t {
  uint8_t *buffer;
  int buffer_len;
  int status_code;

} http_rest_recv_buffer_t;

/**
 * @brief Same as above but with cJSON object as response. Is useful only if the
 * response is expected to be in json and the user does not want to handle
 * assigning it to cjson
 *
 */
typedef struct http_rest_recv_json_t {
  cJSON *json;
  int status_code;

} http_rest_recv_json_t;

/**
 * @brief Struct used to provide custom_headers to the request
 *
 */
typedef struct {
  const char *key;
  const char *value;
} http_header_t;