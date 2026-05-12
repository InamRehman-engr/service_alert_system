
#include "iotcore_server_apis.h"
#include "cJSON.h"
#include "connectivity.h"
#include "esp_event.h"
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "freertos/queue.h"
#include "freertos/semphr.h"
#include "freertos/task.h"
#include "iotcore_events.h"
#include "myJSON.h"
#include "sdkconfig.h"
#include <esp_netif.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define HTTP_VERSION 1

#define HTTP_FAILED_COUNT_MAX 10
#define STR_HELPER(x) #x
#define STR(x) STR_HELPER(x)

static const char *TAG_HTTP = "HTTP";
#ifdef CONFIG_ENABLE_IOTCORE_SERVER_API_EMBED_JSON
extern const uint8_t assets_beef_txt_start[] asm(
    "_binary_beef_txt_start"); // Start of the embedded binary data
extern const uint8_t assets_beef_txt_end[] asm(
    "_binary_beef_txt_end"); // End of the embedded binary data
#endif

char clientID_get = 1;
int32_t http_failedCount = 0;

static Http_credentials_t Global_http_credentials;

__attribute__((weak)) void print_unittest_label() {}

// This will publish the onboarding complete event.
void parseAndSetClientId(char *str) {
  cJSON *root;
  cJSON *obj;
  cJSON *data;
  root = cJSON_Parse(str);
  if (cJSON_IsObject(root)) {
    obj = cJSON_GetObjectItem(root, "success");
    if (cJSON_IsBool(obj)) {
      if (cJSON_IsTrue(obj) == true) {
        data = cJSON_GetObjectItem(root, "data"); // get data object
        if (cJSON_IsObject(data)) {
          obj = cJSON_GetObjectItem(data, "id");
          if (cJSON_IsNumber(obj)) {
            post_iotcore_app_event(SYSTEM_IOTCORE_API_GOT_CLIENT_ID,
                                   &obj->valueint, sizeof(int));
            ESP_LOGI(TAG_HTTP, "got the cliendID [%d]\n\n\n", obj->valueint);
            clientID_get = 0;
#ifdef CONFIG_UNITTEST_ENABLE_ALL
            print_unittest_label();
#endif
          }
        }
      }
    }
  }
  cJSON_Delete(root);
}

char *http_get_accessToken() {
  ESP_LOGI(TAG_HTTP, "http_get_accessToken.....");
  char *token = NULL;
  char host[100];
  cJSON *root = cJSON_CreateObject();
  cJSON_AddStringToObject(root, "email",
                          Global_http_credentials.device_api_username);
  cJSON_AddStringToObject(root, "password",
                          Global_http_credentials.device_api_password);
  cJSON_AddFalseToObject(root, "remember");
  char *post_data = cJSON_PrintUnformatted(root);
  snprintf(host, sizeof(host), "%s/auth",
           Global_http_credentials.http_api_host);
  http_rest_recv_buffer_t response_buffer = {0};
  esp_err_t err = http_rest_client_post(host, post_data, strlen(post_data),
                                        &response_buffer, NULL, 0);
  cJSON_Delete(root);
  if (err == ESP_OK) {
    ESP_LOGI(TAG_HTTP, "Status = %d, content_length = %d",
             response_buffer.status_code, response_buffer.buffer_len);

    if (response_buffer.buffer != NULL) {
      cJSON *root = cJSON_Parse((char *)response_buffer.buffer);

      if (cJSON_IsObject(root)) {
        cJSON *obj = cJSON_GetObjectItem(root, "success");

        if (cJSON_IsBool(obj) && cJSON_IsTrue(obj)) {
          cJSON *data = cJSON_GetObjectItem(root, "data");

          if (cJSON_IsObject(data)) {
            cJSON *accessTokenObj = cJSON_GetObjectItem(data, "accessToken");

            if (cJSON_IsString(accessTokenObj)) {
              token = (char *)malloc(
                  strlen(accessTokenObj->valuestring) +
                  1); // This will need to be freed at task deletion
              strcpy(token, accessTokenObj->valuestring);
            }
          }
        }
      }
      cJSON_Delete(root);
    }
  } else {
    ESP_LOGE(TAG_HTTP, "Error %d  Status = %d, content_length = %d", err,
             response_buffer.status_code, response_buffer.buffer_len);
  }

  if (response_buffer.status_code == 0) {
    /// TODO: Introduce a way here to tell the application that server
    /// connection failed with status 0
  }

  if (response_buffer.buffer_len > 0) {
    ESP_LOG_BUFFER_HEXDUMP(TAG_HTTP, response_buffer.buffer,
                           response_buffer.buffer_len, ESP_LOG_DEBUG);
  }

  http_failedCount++;
  if (http_failedCount >= HTTP_FAILED_COUNT_MAX) {
    http_failedCount = 0;
  }

  http_rest_client_cleanup(&response_buffer);
  ESP_LOGI(TAG_HTTP, "................http_get_accessToken");
  return token;
}

void http_get_clientID(char *accessToken, device_info_t *onboardingData) {
  ESP_LOGI(TAG_HTTP, "http_get_clientID.....");
  char post_data1[120];
  sprintf(post_data1,
          "serial=%s&pin_code=%s&controller=true&device_type=%d&hw_ver=%s",
          onboardingData->registrationNumber, onboardingData->devicePin,
          onboardingData->deviceTypeId, onboardingData->hardwareVersion);
  char host[100];
  sprintf(host, "%s/device/create", Global_http_credentials.http_api_host);
  http_header_t headers[] = {
      {.key = "Content-Type", .value = "application/x-www-form-urlencoded"},
      {.key = "accept", .value = "application/applicationjson"},
      {.key = "authorization", .value = accessToken}};
  http_rest_recv_buffer_t response_buffer = {0};
  esp_err_t err = http_rest_client_post(
      host, post_data1, strlen(post_data1), &response_buffer, headers,
      sizeof(headers) / sizeof(http_header_t));
  if (err == ESP_OK) {
    ESP_LOGI(TAG_HTTP, "Status = %d, content_length = %d",
             response_buffer.status_code, response_buffer.buffer_len);

    if (response_buffer.buffer != NULL) {
      ESP_LOGD(TAG_HTTP, ">\n[[[%.*s]]]\n", (int)response_buffer.buffer_len,
               (char *)response_buffer.buffer);
      parseAndSetClientId((char *)response_buffer.buffer);
    }
  }

  if (response_buffer.status_code == 0) {
    /// TODO: Introduce a way here to tell the application that server
    /// connection failed with status 0
  }

  http_failedCount++;
  if (http_failedCount >= HTTP_FAILED_COUNT_MAX) {
    http_failedCount = 0;
  }

  http_rest_client_cleanup(&response_buffer);
  ESP_LOGI(TAG_HTTP, "................http_get_clientID");
}

bool http_task_active = false;

/// TODO: this could be improved
void http_get_set_client_id(void *pvParameters) {
  device_info_t *onboardingData = (device_info_t *)pvParameters;
  char *accessToken = NULL;
  ESP_LOGI(TAG_HTTP, "http_get_set before Free memory: %ld bytes",
           esp_get_free_heap_size());
  while (true) {
    waitDeviceHasInternet();
    {
      if (accessToken == NULL) {
#ifdef CONFIG_ENABLE_IOTCORE_SERVER_API_EMBED_JSON
        accessToken = assets_beef_txt_start;
#else
        accessToken = http_get_accessToken(); // Token will be set here
#endif
      } else if (clientID_get == 1) {
        ESP_LOGI(TAG_HTTP, "create device in cloud");
        http_get_clientID(accessToken,
                          onboardingData); // Token will be used here
      } else {
        ESP_LOGE(TAG_HTTP, "http_get_set after Free memory: %ld bytes",
                 esp_get_free_heap_size());
        http_task_active = 0;
        if (accessToken != NULL) {
#ifndef CONFIG_ENABLE_IOTCORE_SERVER_API_EMBED_JSON
          accessToken = NULL; // This is not dynamically allocated memory
#else
          free(accessToken);
#endif
        }
        vTaskDelete(NULL);
      }
      vTaskDelay(pdMS_TO_TICKS(5000));
    }
    ESP_LOGI(TAG_HTTP, "................http_get_set");
  }
}

void iotcore_server_apis_task(Http_credentials_t http_variables,
                              device_info_t *onboardingData) {
  BaseType_t ts = 0;
  if (http_task_active == false) {
    memcpy(&Global_http_credentials, &http_variables,
           sizeof(Http_credentials_t));
    http_task_active = true;
    vTaskDelay(10 / portTICK_PERIOD_MS);
    // Stack usage of this task tested with uxTaskGetStackHighWaterMark results
    // in usage of 3820 bytes max. If in future this task is modified be sure to
    // calculate the new task size
    ts = xTaskCreate(http_get_set_client_id, "http_get_set_client_id", 4 * 1024,
                     onboardingData, 5, NULL);

    if (pdPASS != ts) {
      post_task_create_failed_event(__FILE__, __LINE__,
                                    esp_get_free_heap_size());
    }
  }
}