

#include "errorHandling.h"
#include "esp_eap_client.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "freertos/queue.h"
#include "freertos/semphr.h"
#include "freertos/task.h"
#include "nvs_flash.h"
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#if (ESP_IDF_VERSION_MAJOR > 4)
#include "esp_netif.h"
#else
#if (ESP_IDF_VERSION_MAJOR == 4) && (ESP_IDF_VERSION_MINOR >= 1)
#include "esp_netif.h"
#else
#include "tcpip_adapter.h"
#endif
#endif
#include "esp_event.h"
#include "esp_smartconfig.h"

#include "lwip/apps/sntp.h"
#include "lwip/dns.h"
#include "lwip/err.h"
#include "lwip/netdb.h"
#include "lwip/sockets.h"

#include "connectivity.h"
#include "esp_tls.h"
#include "iotcore_events.h"

#include "driver/gpio.h"
#include "driver/ledc.h"
#include "nvs.h"

#include "esp_http_client.h"
#include <esp_http_server.h>
#include <sys/param.h>

#include "cJSON.h"
#include "myJSON.h"

#include "esp_sleep.h"
#include "url_encoding.h"

#include "DC-codes.h"
#include "sysinfo.h"

extern esp_err_t saveKeyValueInFlash_int32(char *key, int32_t data);

const uint8_t *api_server_cert_pem = NULL;

dc_codes_bits error_codes_dc = {.all = 0};
dc_codes_bits error_codes_dc_reported = {.all = 0};

char slack_api_url[] = "https://hooks.slack.com/services/T45BZAL48/BPH8HH5SP/"
                       "sqRXz3tEUilxPasWYcwoYloE";
char slackAPiSetBuffer[1500];
// TODO: Allocate with malloc where required and fix checks for allocation
int32_t slackAPiSetBufferLen = 0;

esp_http_client_handle_t clientSlack;
extern const int INTERNET_CONNECTED_BIT;

#define TAG "SLACK"

#ifdef PRODUCTION_CODE
#define printd(...)
#else
#define printd printf
#endif

esp_err_t _http_event_handle_slack(esp_http_client_event_t *evt) {
  switch (evt->event_id) {
  case HTTP_EVENT_ERROR:
    ESP_LOGI(TAG, "slackHTTP_EVENT_ERROR");
    break;
  case HTTP_EVENT_ON_CONNECTED:
    ESP_LOGI(TAG, "slackHTTP_EVENT_ON_CONNECTED");
    break;
  case HTTP_EVENT_HEADER_SENT:
    ESP_LOGI(TAG, "slackHTTP_EVENT_HEADER_SENT");
    break;
  case HTTP_EVENT_ON_HEADER:
    ESP_LOGI(TAG, "slackHTTP_EVENT_ON_HEADER");
    printd("%.*s", evt->data_len, (char *)evt->data);
    break;
  case HTTP_EVENT_ON_DATA:
    ESP_LOGI(TAG, "slackHTTP_EVENT_ON_DATA, len=%d", evt->data_len);
    if (!esp_http_client_is_chunked_response(evt->client)) {
      printd("%.*s", evt->data_len, (char *)evt->data);
      if (strcmp(slackAPiSetBuffer, "\0") != 0) {
        sprintf(&slackAPiSetBuffer[slackAPiSetBufferLen], "%.*s\n",
                evt->data_len, (char *)evt->data);
        slackAPiSetBufferLen += evt->data_len;
      }
    } else {
      printd("headers->>>>>>>%.*s\n", evt->data_len, (char *)evt->data);
    }

    break;
  case HTTP_EVENT_ON_FINISH:
    ESP_LOGI(TAG, "slackHTTP_EVENT_ON_FINISH   [%ld]", slackAPiSetBufferLen);
    if (strcmp(slackAPiSetBuffer, "\0") != 0) {
      printd("[[[%.*s]]]", (int)slackAPiSetBufferLen,
             (char *)slackAPiSetBuffer);
    }
    break;
  case HTTP_EVENT_DISCONNECTED:
    ESP_LOGI(TAG, "slackHTTP_EVENT_DISCONNECTED");
    break;
  case HTTP_EVENT_REDIRECT:
    ESP_LOGI(TAG, "slackHTTP_EVENT_REDIRECT");
    break;
  }
  return ESP_OK;
}
char *uri_data;
char uri_data_urlsafe[500];
char pvParameters_urlsafe[500];
char post_data[510];
char post_data_urlsafe[500];
char json_buffer[500];

int32_t post_to_slackarray_count = 0;
char *post_to_slackarray[post_to_slackarray_max];

void http_posterror_set_uri(char *uri, const uint8_t *cert_pem) {
  for (int i = 0; i < post_to_slackarray_max; i++) {
    post_to_slackarray[i] = NULL;
  }
  uri_data = uri;
  api_server_cert_pem = cert_pem;
  /// TODO: test this
}
esp_err_t http_postToSlack(char *pvParameters) {
  for (int i = 0; i < post_to_slackarray_max; i++) {
    if (post_to_slackarray[i] == NULL) {
      int len = strlen(pvParameters);
      post_to_slackarray[i] = malloc(len);
      strcpy(post_to_slackarray[i], pvParameters);
      post_to_slackarray_count++;
      ESP_LOGI(TAG, "http_postToSlack..... \ninsert data at index %d\n", i);
      return ESP_OK;
    }
  }
  return ESP_FAIL;

  esp_err_t err = ESP_FAIL;
  ESP_LOGI(TAG, "http_postToSlack.....");
  static SemaphoreHandle_t http_postToSlack_semaphore;
  if (http_postToSlack_semaphore == NULL)
    http_postToSlack_semaphore = xSemaphoreCreateMutex();

  if (xSemaphoreTake(http_postToSlack_semaphore, 3000 / portTICK_PERIOD_MS) ==
      pdFALSE)
    return err;

  if (uri_data != NULL && (checkDeviceHasInternet())) {
    slackAPiSetBufferLen = 0;

    esp_http_client_config_t config = {
        .url = uri_data,
        .event_handler = _http_event_handle_slack,
        .buffer_size_tx = 1024,
    };

    if (api_server_cert_pem) {
      config.cert_pem = (const char *)api_server_cert_pem;
      config.port = 443;
      config.transport_type = HTTP_TRANSPORT_OVER_SSL;
    }

    printd("url : %s \n", config.url);
    // if (config.cert_pem)
    //    printd("cert_pem : %s\n", config.cert_pem);
    esp_http_client_handle_t clientSlack = esp_http_client_init(&config);

    esp_http_client_set_method(clientSlack, HTTP_METHOD_POST);

    // esp_http_client_set_header(clientSlack, "accept", "application/json");
    // esp_http_client_set_header(clientSlack, "host", HTTP_API_HOST);
    esp_http_client_set_header(clientSlack, "Content-Type",
                               "application/x-www-form-urlencoded");

    url_encode(pvParameters, pvParameters_urlsafe, 0);
    sprintf(post_data, "data=%s", pvParameters_urlsafe);

    // url_encode(slack_api_url, uri_data_urlsafe, 0);
    // url_encode(pvParameters, pvParameters_urlsafe, 0);
    // sprintf(post_data, "url=%s&data=%s", uri_data_urlsafe,
    // pvParameters_urlsafe);

    esp_http_client_set_post_field(clientSlack, post_data, strlen(post_data));

    esp_err_t err = esp_http_client_perform(clientSlack);

    if (err == ESP_OK) {
      ESP_LOGI(TAG, "Status = %d, content_length = %lld",
               esp_http_client_get_status_code(clientSlack),
               esp_http_client_get_content_length(clientSlack));
    } else {
      ESP_LOGE(TAG, "ERRor %d  Status = %d, content_length = %lld", err,
               esp_http_client_get_status_code(clientSlack),
               esp_http_client_get_content_length(clientSlack));
    }

    esp_http_client_cleanup(clientSlack);
    // free(httpSetBuffer);
    ESP_LOGI(TAG, "................http_getslack");
  }
  xSemaphoreGive(http_postToSlack_semaphore);

  return err;
}

int postMsgOnMQTT(char *string) {
  char topic[30];
  int ret = 0;
  sprintf(topic, "debugv0");
  post_mqtt_publish_event(string, strlen(string), topic, 0, 1);
  return ret;
}

int postMsgOndebugvp(char *string) {
  char topic[30];
  int ret = 0;
  sprintf(topic, "debugvp");
  post_mqtt_publish_event(string, strlen(string), topic, 0, 1);
  return ret;
}

#define SOURCE_PATH_SIZE 27

void addDeviceIDandFirmware(cJSON *object, int32_t deviceID) {
  if (object == NULL)
    return;
  char *device_id = NULL;
  asprintf(&device_id, "%ld", deviceID);
  cJSON_AddItemToObject(object, "did", cJSON_CreateString(device_id));
  cJSON_AddItemToObject(object, "fv",
                        cJSON_CreateString(getDeviceInfo()->firmwareVersion));
  free(device_id);
}

void MY_ERROR_HANDLING_watchdogflow(char *taskname, int32_t clientID) {
  cJSON *root;
  root = cJSON_CreateObject();
  int mqttpost = 0;
  char *json_str;
  addDeviceIDandFirmware(root, clientID);
  if (taskname == NULL) {
    cJSON_AddItemToObject(root, "t_wdovflow", cJSON_CreateString("unknown"));
  } else {
    cJSON_AddItemToObject(root, "t_wdovflow", cJSON_CreateString(taskname));
  }
  json_str = cJSON_Print(root);
  mqttpost = postMsgOnMQTT(json_str);
  if (mqttpost < 0)
    http_postToSlack(json_str);

  if (json_str)
    free(json_str);
  cJSON_Delete(root);

  error_codes_dc.flags.watchdogoverflow = true;
#ifdef PRODUCTION_CODE
  MY_ERROR_HANDLING_PostDcERRORCode(error_codes_dc.all);
#endif
}

void MY_ERROR_HANDLING_UARTError(char *error, int32_t clientID) {
  error_codes_dc.flags.UARTError = true;
#ifdef PRODUCTION_CODE
  MY_ERROR_HANDLING_PostDcERRORCode(error_codes_dc.all);
#else
  if (error_codes_dc_reported.flags.UARTError == 0) {
    cJSON *root;
    int mqttpost = -1;
    root = cJSON_CreateObject();
    char *json_str;
    addDeviceIDandFirmware(root, clientID);
    cJSON_AddItemToObject(root, "uart_f", cJSON_CreateString(error));

    json_str = cJSON_Print(root);
    mqttpost = postMsgOnMQTT(json_str);
    if (mqttpost < 0) {
      http_postToSlack(json_str);
      error_codes_dc_reported.flags.UARTError = true;
    } else {
      error_codes_dc_reported.flags.UARTError = true;
    }

    if (json_str)
      free(json_str);
    cJSON_Delete(root);
  }
#endif
}

void MY_ERROR_HANDLING_SlaveDeviceError(char *error, int32_t clientID) {
  error_codes_dc.flags.slaveDeviceError = true;
#ifdef PRODUCTION_CODE
  MY_ERROR_HANDLING_PostDcERRORCode(error_codes_dc.all);
#else
  if (error_codes_dc_reported.flags.slaveDeviceError == 0) {
    cJSON *root;
    int mqttpost = -1;
    root = cJSON_CreateObject();
    char *json_str;
    addDeviceIDandFirmware(root, clientID);
    cJSON_AddItemToObject(root, "sde", cJSON_CreateString(error));

    json_str = cJSON_Print(root);
    mqttpost = postMsgOnMQTT(json_str);
    if (mqttpost < 0) {
      http_postToSlack(json_str);
      error_codes_dc_reported.flags.slaveDeviceError = true;
    } else {
      error_codes_dc_reported.flags.slaveDeviceError = true;
    }
    if (json_str)
      free(json_str);
    cJSON_Delete(root);
  }
#endif
}

void MY_ERROR_HANDLING_INAError(char *error, int32_t clientID) {
  error_codes_dc.flags.I2CFault = true;
#ifdef PRODUCTION_CODE
  MY_ERROR_HANDLING_PostDcERRORCode(error_codes_dc.all);
#else
  cJSON *root;
  int mqttpost = 0;
  root = cJSON_CreateObject();
  char *json_str;
  addDeviceIDandFirmware(root, clientID);
  cJSON_AddItemToObject(root, "INA_f", cJSON_CreateString(error));

  json_str = cJSON_Print(root);
  mqttpost = postMsgOnMQTT(json_str);
  if (mqttpost < 0)
    http_postToSlack(json_str);

  if (json_str)
    free(json_str);
  cJSON_Delete(root);

#endif
}

void MY_ERROR_HANDLING_CommErr_Between_Motor_And_Tanksensor(char *error,
                                                            int32_t clientID) {
  error_codes_dc.flags.CommunicationErrBetweenMotorAndTank = true;
#ifdef PRODUCTION_CODE
  MY_ERROR_HANDLING_PostDcERRORCode(error_codes_dc.all);
#else
  cJSON *root;
  int mqttpost = 0;
  root = cJSON_CreateObject();
  char *json_str;
  addDeviceIDandFirmware(root, clientID);
  cJSON_AddItemToObject(root, "Tank_comm_f", cJSON_CreateString(error));

  json_str = cJSON_Print(root);
  mqttpost = postMsgOnMQTT(json_str);
  if (mqttpost < 0)
    http_postToSlack(json_str);

  if (json_str)
    free(json_str);
  cJSON_Delete(root);

#endif
}

void MY_ERROR_HANDLING_Reading_Tanksensor(char *error, int32_t clientID) {
  error_codes_dc.flags.reading_tanksensor = true;
#ifdef PRODUCTION_CODE
  MY_ERROR_HANDLING_PostDcERRORCode(error_codes_dc.all);
#else
  cJSON *root = NULL;
  int mqttpost = 0;
  root = cJSON_CreateObject();
  if (root) {
    char *json_str;
    addDeviceIDandFirmware(root, clientID);
    cJSON_AddItemToObject(root, "tank_sensor_err", cJSON_CreateString(error));

    json_str = cJSON_Print(root);
    mqttpost = postMsgOnMQTT(json_str);
    if (mqttpost < 0)
      http_postToSlack(json_str);

    if (json_str)
      free(json_str);
  } else {
    ESP_LOGE(TAG, " cJSON_CreateObject no object created");
  }
  cJSON_Delete(root);
#endif
}

void MY_ERROR_HANDLING_INAoutOfRange(float vbat, float current,
                                     int32_t clientID) {
  error_codes_dc.flags.OutOfRange = true;
#ifdef PRODUCTION_CODE
  MY_ERROR_HANDLING_PostDcERRORCode(error_codes_dc.all);
#else
  cJSON *root;
  cJSON *object;
  int mqttpost = 0;
  root = cJSON_CreateObject();
  char *json_str;
  addDeviceIDandFirmware(root, clientID);
  object = cJSON_CreateObject();
  cJSON_AddItemToObject(root, "INA_r", object);
  myJSON_AddRawToObject(object, "vbat", vbat, 3);
  myJSON_AddRawToObject(object, "curr", current, 3);

  json_str = cJSON_Print(root);
  mqttpost = postMsgOnMQTT(json_str);
  if (mqttpost < 0)
    http_postToSlack(json_str);

  if (json_str)
    free(json_str);
  cJSON_Delete(root);
#endif
}

void MY_ERROR_HANDLING_PostBattery(float voltage, int32_t clientID) {
  error_codes_dc.flags.BATTLOW = true;
#ifdef PRODUCTION_CODE
  MY_ERROR_HANDLING_PostDcERRORCode(error_codes_dc.all);
#else
  cJSON *root;
  root = cJSON_CreateObject();
  char *json_str;
  addDeviceIDandFirmware(root, clientID);
  myJSON_AddRawToObject(root, "vbat", (voltage), 3);
  json_str = cJSON_Print(root);
  postMsgOnMQTT(json_str);
  if (json_str)
    free(json_str);
  cJSON_Delete(root);
#endif
}

void MY_ERROR_HANDLING_PostDcERRORCode(int dc_code, int32_t clientID) {
  if (dc_code != error_codes_dc_reported.all) {
    if (dc_code == 0) {
      error_codes_dc_reported.all = dc_code;
    } else {
      int diff_error_code =
          (dc_code ^ error_codes_dc_reported.all); // post only which are new
      printd("error code %X ,   reported  %lx   ", dc_code,
             error_codes_dc_reported.all);
      printd("diff    %X \n ", diff_error_code);
      cJSON *root;
      root = cJSON_CreateObject();
      char *json_str;
      addDeviceIDandFirmware(root, clientID);
      cJSON_AddItemToObject(root, "dc", cJSON_CreateNumber(diff_error_code));

      json_str = cJSON_Print(root);
      int mqttpost = 0;
      mqttpost = postMsgOndebugvp(json_str);
      if (mqttpost > 0) {
        error_codes_dc_reported.all = dc_code;
        printd("successfully posted on slack");
      } else {
        printd(
            "                                     error wile posting on slack");
      }

      if (json_str)
        free(json_str);
      cJSON_Delete(root);
    }
  }
}

void errorHandlingEventHandler(void *event_handler_arg,
                               esp_event_base_t event_base, int32_t event_id,
                               void *event_data) {
  !event_handler_arg ? 0 : ({ return; });
  cJSON *root;
  cJSON *object;
  char *json_str;
  root = cJSON_CreateObject();
  addDeviceIDandFirmware(root, *(int32_t *)event_handler_arg);
  switch (event_id) {
  case TASK_CREATE_FAILED:
    thread_create_failed_t *task_thread_fail_data =
        (thread_create_failed_t *)event_data;
    cJSON_AddNumberToObject(root, "heap", task_thread_fail_data->freeheap);
    object = cJSON_CreateObject();
    cJSON_AddItemToObject(root, "t_fail", object);
    cJSON_AddItemToObject(
        object, "file",
        cJSON_CreateString(task_thread_fail_data->file + SOURCE_PATH_SIZE));
    cJSON_AddNumberToObject(object, "line", task_thread_fail_data->line);
    break;
  case THREAD_FAILED:
    break;
  case GENERAL_ESP_ERROR_CODE: // This will contain events whose data is int
                               // castable
    char ec_string[8] = "0";
    sprintf(ec_string, "%X", *(int *)event_data);
    cJSON_AddItemToObject(root, "ec", cJSON_CreateString(ec_string));
    break;
  case TASK_STACK_OVERFLOW:
    cJSON_AddItemToObject(root, "t_sof",
                          cJSON_CreateString((char *)event_data));
    break;
  case MEMORY_ALLOCATION_FAILED:
    cJSON_AddItemToObject(root, "ma_fail",
                          cJSON_CreateString((char *)event_data));
    break;
  case OTA_FAILED:
    object = cJSON_CreateObject();
    cJSON_AddItemToObject(root, "ota_f", object);
    cJSON_AddItemToObject(object, "fv", cJSON_CreateString((char *)event_data));
  default:
    // Not my event
    break;
  }
  json_str = cJSON_Print(root);
  postMsgOnMQTT(json_str);
  if (json_str)
    free(json_str);
  cJSON_Delete(root);
}

esp_err_t start_iotcore_error_handler(int32_t *clientID) {
  return register_iotcore_error_event(TASK_CREATE_FAILED,
                                      errorHandlingEventHandler, clientID) ||
         register_iotcore_error_event(THREAD_FAILED, errorHandlingEventHandler,
                                      clientID) ||
         register_iotcore_error_event(GENERAL_ESP_ERROR_CODE,
                                      errorHandlingEventHandler, clientID) ||
         register_iotcore_error_event(TASK_STACK_OVERFLOW,
                                      errorHandlingEventHandler, clientID) ||
         register_iotcore_error_event(MEMORY_ALLOCATION_FAILED,
                                      errorHandlingEventHandler, clientID) ||
         register_iotcore_error_event(OTA_FAILED, errorHandlingEventHandler,
                                      clientID);
}
