
#pragma once

#include "esp_http_client.h"
#include "esp_log.h"
#include "http_rest_types.h"
#include <stdio.h>
#include <string.h>

/**
 * @brief Event handler for data of http requests internally it is used by all
 * requests to write data to the out buffer and provide status code
 *
 * @param event_data Internal esp32 http structure that provides multiple events
 * only one of which is data event
 * @return esp_err_t
 */
esp_err_t http_event_handler(esp_http_client_event_t *event_data);
