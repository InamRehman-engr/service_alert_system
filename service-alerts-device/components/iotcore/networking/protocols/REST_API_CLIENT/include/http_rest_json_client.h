
#pragma once

#include "cJSON.h"
#include "http_rest_client.h"

esp_err_t http_rest_client_get_json(char *url,
                                    http_rest_recv_json_t *http_rest_recv_json);
esp_err_t
http_rest_client_delete_json(char *url,
                             http_rest_recv_json_t *http_rest_recv_json);
esp_err_t
http_rest_client_post_json(char *url, cJSON *body_json,
                           http_rest_recv_json_t *http_rest_recv_json);
esp_err_t http_rest_client_put_json(char *url, cJSON *body_json,
                                    http_rest_recv_json_t *http_rest_recv_json);

void http_rest_client_cleanup_json(http_rest_recv_json_t *http_rest_recv_json);