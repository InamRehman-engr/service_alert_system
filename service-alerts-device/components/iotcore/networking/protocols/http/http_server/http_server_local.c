#include "http_server_local.h"
#include "esp_eap_client.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_smartconfig.h"
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
#include "esp_timer.h"
#include <esp_http_server.h>
#include <sys/param.h>

#include "cJSON.h"
#include "iotcore_events.h"

#include "DC-codes.h"
#include "url_encoding.h"

#include "sysinfo.h"
#ifdef CONFIG_ENABLE_WIFI
#include "wifi_manager_provisioning_api.h"
#endif

#ifdef CONFIG_ENABLE_OTA
#include "firmware_check.h"
#include "http_ota_handler.h"
#include "native_ota.h"
/* Scratch buffer size */
#define OTA_BUFSIZE 6144
char ota_buffer[OTA_BUFSIZE] = {0};
#endif

static const char *TAG = "HSERVER";

#ifdef PRODUCTION_CODE
#define printd(...)
#else
#define printd printf
#endif

httpd_handle_t Http_server = NULL;
int32_t wifi_ssids_check_new_wifi = -1;

int32_t wifi_ssids_count_before_new_wifi = -1;

int GotTheWifiNewPassword = 0;

/* An HTTP POST handler */
static esp_err_t wifi_post_handler(httpd_req_t *req) {
  char buf[200];

  // device_mode_setupModeWifi_timer = esp_timer_get_time()/1000000;    // reset
  // the timer
  cJSON *response;
  char *json_str;

  char responseString[1000];

  sprintf(responseString, "Error");

  int ret, remaining = req->content_len;
  response = cJSON_CreateObject();

  while (remaining > 0) {
    /* Read the data for the request */
    if ((ret = httpd_req_recv(req, buf, MIN(remaining, sizeof(buf)))) <= 0) {
      if (ret == HTTPD_SOCK_ERR_TIMEOUT) {
        /* Retry receiving if timeout occurred */
        continue;
      }
      cJSON_Delete(response);
      return ESP_FAIL;
    }
    buf[ret] = 0; // termination char in str;

    /* Log data received */
    ESP_LOGI(TAG, "\n=========== RECEIVED DATA ==========");
    ESP_LOGI(TAG, "%.*s", ret, buf);
    ESP_LOGI(TAG, "uri->%s", req->uri);
    ESP_LOGI(TAG, "====================================");
    remaining -= ret;
  }

  httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
  httpd_resp_set_hdr(req, "Access-Control-Allow-Methods", "POST");
  httpd_resp_set_hdr(req, "Access-Control-Allow-Headers",
                     "Origin, Content-Type, X-Auth-Token");
  httpd_resp_set_type(req, "application/json");

#ifdef CONFIG_ENABLE_WIFI
  char param[32] = {0};
  char p_pass[50] = {0};
  char p_ssid[50] = {0};
  char p_pin[10] = {0};
  char user_id[50] = {0};

  if (httpd_query_key_value(buf, "ssid", param, sizeof(param)) == ESP_OK) {
    url_decode(param, p_ssid, 0);
    ESP_LOGI(TAG, "Found URL query parameter => ssid=%s '%s'", param, p_ssid);
  }
  if (httpd_query_key_value(buf, "pass", param, sizeof(param)) == ESP_OK) {
    url_decode(param, p_pass, 0);
    ESP_LOGI(TAG, "Found URL query parameter => pass=%s '%s'", param, p_pass);
  }
  if (httpd_query_key_value(buf, "pin", param, sizeof(param)) == ESP_OK) {
    url_decode(param, p_pin, 0);
    ESP_LOGI(TAG, "Found URL query parameter => pin=%s '%s'", param, p_pin);
  }
  if (httpd_query_key_value(buf, "user_id", param, sizeof(param)) == ESP_OK) {
    url_decode(param, user_id, 0);
    ESP_LOGI(TAG, "Found URL query parameter => user_id=%s '%s'", param,
             user_id);
  }

  if (strlen(p_pin) != 0) {
    ESP_LOGI(TAG, "PIN  %s", p_pin);
    if (strlen(p_pin) == 3) {
      char p_pin_temp[50] = {0};
      sprintf(p_pin_temp, "0%s", p_pin);
      strncpy(p_pin, p_pin_temp, 4);
      ESP_LOGI(TAG, "PIN  %s", p_pin);
    }
    if (strcmp(p_pin, getDeviceInfo()->devicePin)) {
      sprintf(responseString, "PIN is not valid");
    } else {
      cJSON_AddItemToObject(
          response, "serial",
          cJSON_CreateString(getDeviceInfo()->registrationNumber));
      if (strlen(p_ssid) != 0) {
        api_wifi_manager_connect_to_wifi(p_ssid, p_pass);
        ESP_LOGI("WIFI", "Wifi and password is recived ");

        ESP_LOGI(TAG, "Wifi SSID %s", p_ssid);
        ESP_LOGI(TAG, "Wifi PASS %s", p_pass);
        ESP_LOGI(TAG, "-");
        vTaskDelay(200 / portTICK_PERIOD_MS);

        sprintf(responseString, "success");
      } else if (strlen(p_ssid) != 0) {
        sprintf(responseString, "SSID is not valid");
      }
    }
  }
  /* Send back the same data */
  // httpd_resp_send_chunk(req, buf, ret);

  cJSON_AddItemToObject(response, "response",
                        cJSON_CreateString(responseString));

  // End response
  json_str = cJSON_Print(response);
  printd("%s", json_str);
  httpd_resp_send(req, json_str, strlen(json_str));
  if (json_str)
    free(json_str);
  cJSON_Delete(response);
#else
  httpd_resp_send(req, "Not allowed when wifi not enabled",
                  strlen("Not allowed when wifi not enabled"));
#endif
  return ESP_OK;
}

static const httpd_uri_t wifi = {.uri = "/wifi",
                                 .method = HTTP_POST,
                                 .handler = wifi_post_handler,
                                 .user_ctx = NULL};

esp_err_t wifi_form_handler(httpd_req_t *req) {
  // Handler for network page
  extern const char wifi_html_start[] asm("_binary_wifi_html_start");
  extern const char wifi_html_end[] asm("_binary_wifi_html_end");
  const size_t html_len = wifi_html_end - wifi_html_start;
  httpd_resp_set_type(req, "text/html");
  return httpd_resp_send(req, wifi_html_start, html_len);
}
#ifdef CONFIG_ENABLE_NETWORK_UI
static const httpd_uri_t network = {.uri = "/network_ui",
                                    .method = HTTP_GET,
                                    .handler = wifi_form_handler,
                                    .user_ctx = NULL};
#endif
esp_err_t ota_form_handler(httpd_req_t *req) {
  extern const char ota_upload_html_start[] asm(
      "_binary_ota_upload_html_start");
  extern const char ota_upload_html_end[] asm("_binary_ota_upload_html_end");
  const size_t html_len = ota_upload_html_end - ota_upload_html_start;

  httpd_resp_set_type(req, "text/html");
  return httpd_resp_send(req, ota_upload_html_start, html_len);
}
#ifdef CONFIG_ENABLE_FIRMWARE_UPLOAD_UI
static const httpd_uri_t ota_ui = {.uri = "/firmware_upload_ui",
                                   .method = HTTP_GET,
                                   .handler = ota_form_handler,
                                   .user_ctx = NULL};
#endif
/* An HTTP GET handler */
esp_err_t scan_get_handler(httpd_req_t *req) {
  // device_mode_setupModeWifi_timer = esp_timer_get_time()/1000000;    // reset
  // the timer

  /* Set some custom headers */
  httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
  httpd_resp_set_hdr(req, "Access-Control-Allow-Methods",
                     "GET, POST, PATCH, PUT, DELETE, OPTIONS");
  httpd_resp_set_hdr(req, "Access-Control-Allow-Headers",
                     "Origin, Content-Type, X-Auth-Token");
  httpd_resp_set_type(req, "application/json");

  ESP_LOGI(TAG, "Added header in responce");

  /* Send response with custom headers and body set as the
   * string passed in user context*/
  // const char* resp_str = (const char*) req->user_ctx;
  ESP_LOGI(TAG, "scan wait over");
#ifdef CONFIG_ENABLE_WIFI
  char *json_str;
  uint8_t numberOfScanAPs = 0;
  wifi_ap_record_t *wifi_List =
      api_wifi_manager_get_scan_list(&numberOfScanAPs);
  cJSON *response;
  cJSON *ssids_json = NULL;
  cJSON *ssid_json = NULL;
  response = cJSON_CreateObject();
  cJSON_AddItemToObject(response, "response", cJSON_CreateString("success"));
  ssids_json = cJSON_CreateArray();
  cJSON_AddItemToObject(response, "SSIDS", ssids_json);

  for (int i = 0; i < numberOfScanAPs; i++) {
    ssid_json = cJSON_CreateObject();
    cJSON_AddItemToArray(ssids_json, ssid_json);
    cJSON_AddItemToObject(ssid_json, "ssid",
                          cJSON_CreateString((char *)wifi_List[i].ssid));
    cJSON_AddItemToObject(ssid_json, "rssi",
                          cJSON_CreateNumber(wifi_List[i].rssi));
  }
  json_str = cJSON_PrintUnformatted(response);

  cJSON_Delete(response);
  httpd_resp_send(req, json_str, strlen(json_str));

  vTaskDelay(500 / portTICK_PERIOD_MS);
  if (json_str) {
    free(json_str);
  }
#else
  httpd_resp_send(req, "Not allowed when wifi not enabled",
                  strlen("Not allowed when wifi not enabled"));
#endif
  /* After sending the HTTP response the old HTTP request
   * headers are lost. Check if HTTP request headers can be read now. */
  // if (httpd_req_get_hdr_value_len(req, "Host") == 0) {
  //     ESP_LOGI(TAG, "Request headers lost");
  // }
  return ESP_OK;
}

static const httpd_uri_t scan = {.uri = "/scan",
                                 .method = HTTP_GET,
                                 .handler = scan_get_handler,
                                 .user_ctx = NULL};

#ifdef CONFIG_ENABLE_OTA
void http_server_ota_state(http_ota_state_t state) {
  post_iotcore_app_event(HTTP_OTA_STATE_EVENT, &state,
                         sizeof(http_ota_state_t));
}

static esp_err_t ota_post_handler(httpd_req_t *req) {
  ESP_LOGI(TAG, "OTA API executed with URI : %s", req->uri);
  if (req->method != HTTP_POST) {
    ESP_LOGW(TAG, "Incorrect API method");
    return ESP_FAIL;
  }

  /* Retrieve the pointer to scratch buffer for temporary storage */
  char *buf = (char *)req->user_ctx;
  char param[10];
  if (httpd_req_get_url_query_str(req, buf, OTA_BUFSIZE) != ESP_OK) {
    ESP_LOGW(TAG, "can\'t get query");
    httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "can\'t get query");
    vTaskDelay(pdMS_TO_TICKS(10));
    return ESP_FAIL;
  }
  if (httpd_query_key_value(buf, "pin", param, sizeof(param)) != ESP_OK) {
    ESP_LOGW(TAG, "Pin not found");
    httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Pin not found");
    vTaskDelay(pdMS_TO_TICKS(10));
    return ESP_FAIL;
  }
  if (strcmp(param, getDeviceInfo()->devicePin) != 0) {
    ESP_LOGW(TAG, "Pin mismatch");
    httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Pin mismatch");
    vTaskDelay(pdMS_TO_TICKS(10));
    return ESP_FAIL;
  }

  if (httpd_query_key_value(buf, "fwver", param, sizeof(param)) != ESP_OK) {
    ESP_LOGW(TAG, "Firmware version not found");
    httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST,
                        "Firmware version not found");
    vTaskDelay(pdMS_TO_TICKS(10));
    return ESP_FAIL;
  }
  char rb[2];
  if (httpd_query_key_value(buf, "rollback", rb, sizeof(rb)) != ESP_OK) {
    ESP_LOGW(TAG, "Rollback not found");
    httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Rollback not found");
    vTaskDelay(pdMS_TO_TICKS(10));
    return ESP_FAIL;
  }
  bool rollback = (rb[0] == '1');
  enum firmwareInfo fw_check = validate_firmware_version(
      param, getDeviceInfo()->firmwareVersion, rollback);
  if (fw_check != FIRMWARE_VERSION_OK) {
    ESP_LOGW(TAG, "Firmware version check failed: error code %d", fw_check);
    httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST,
                        "Firmware version validation failed.");
    return ESP_FAIL;
  }
  if (httpd_query_key_value(buf, "hwver", param, sizeof(param)) != ESP_OK) {
    ESP_LOGW(TAG, "Hardware version not found");
    httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST,
                        "Hardware version not found");
    vTaskDelay(pdMS_TO_TICKS(10));
    return ESP_FAIL;
  }
  enum hardwareInfo hw_check =
      validate_hardware_version(param, getDeviceInfo()->hardwareVersion);
  if (hw_check != HARDWARE_VERSION_OK) {
    ESP_LOGW(TAG, "Hardware version check failed: error code %d", hw_check);
    httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST,
                        "Hardware version validation failed.");
    return ESP_FAIL;
  }
  if (httpd_query_key_value(buf, "delta", param, sizeof(param)) != ESP_OK) {
    ESP_LOGW(TAG, "Delta not found");
    httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Delta not found");
    vTaskDelay(pdMS_TO_TICKS(10));
    return ESP_FAIL;
  }
  bool delta = param[0] == '1';
  memset(buf, 0, OTA_BUFSIZE);

  /* Content length of the request gives
   * the size of the file being uploaded */
  int Ota_received_size = 0;
  int Total_ota_size = req->content_len;
  int completed_size = 0;

  if (Total_ota_size <= 0) {
    ESP_LOGE(TAG, "OTA File was not received\n");
    httpd_resp_sendstr(req, "OTA File was not received");
    httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR,
                        "Failed to receive file");
    vTaskDelay(pdMS_TO_TICKS(10));
    return ESP_FAIL;
  }
  http_server_ota_state(HTTP_OTA_DOWNLOAD_STARTED);
  native_ota.init_native_ota(Total_ota_size, NATIVE_OTA_CHECKSUM_NONE, NULL);
  ESP_LOGW(TAG, "Size of OTA File : %d B\n", Total_ota_size);

  while (Total_ota_size > completed_size) {
    if ((Ota_received_size = httpd_req_recv(req, buf, OTA_BUFSIZE)) <= 0) {

      ESP_LOGE(TAG, "File reception failed!");
      /* Respond with 500 Internal Server Error */
      httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR,
                          "Failed to receive file");
      http_server_ota_state(HTTP_OTA_DOWNLOAD_FAILED);
      native_ota.abort_native_ota();
      vTaskDelay(pdMS_TO_TICKS(10));
      return ESP_FAIL;
    }

    completed_size += Ota_received_size;

    // ESP_LOG_BUFFER_HEXDUMP(TAG,buf,ota_received_size,ESP_LOG_WARN);
    ota_packet_t ota_data = {.last_packet = Total_ota_size == completed_size,
                             .packet_length = Ota_received_size,
                             .packet = malloc(Ota_received_size)};
    memcpy(ota_data.packet, buf, Ota_received_size);
    if (xQueueSend(*native_ota.dataqueue, &ota_data, pdMS_TO_TICKS(5000)) !=
        pdTRUE) {
      ESP_LOGE(TAG, "Failed to send Queue.");
      free(ota_data.packet);
      httpd_resp_sendstr(req, "OTA Failed");
      httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR,
                          "Failed to receive file");
      http_server_ota_state(HTTP_OTA_DOWNLOAD_FAILED);
      native_ota.abort_native_ota();
      vTaskDelay(pdMS_TO_TICKS(10));
      return ESP_FAIL;
    }
    vTaskDelay(pdMS_TO_TICKS(10));
    memset(buf, 0, OTA_BUFSIZE);
  }
  http_server_ota_state(HTTP_OTA_DOWNLOAD_SUCCESSFUL);
  /* Redirect onto root to see the updated file list */
  httpd_resp_set_status(req, HTTPD_200);
  httpd_resp_set_hdr(req, "Location", "/");
  httpd_resp_set_hdr(req, "Connection", "close");
  httpd_resp_sendstr(req, "File uploaded successfully");
  vTaskDelay(pdMS_TO_TICKS(10));
  return ESP_OK;
}
static const httpd_uri_t ota = {.uri = "/ota_upload",
                                .method = HTTP_POST,
                                .handler = ota_post_handler,
                                .user_ctx = ota_buffer};
#endif
/* An HTTP GET handler */
esp_err_t network_get_handler(httpd_req_t *req) {
  /* Set some custom headers */
  httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
  httpd_resp_set_hdr(req, "Access-Control-Allow-Methods",
                     "GET, POST, PATCH, PUT, DELETE, OPTIONS");
  httpd_resp_set_hdr(req, "Access-Control-Allow-Headers",
                     "Origin, Content-Type, X-Auth-Token");
  httpd_resp_set_type(req, "application/json");
  ESP_LOGI(TAG, "Added header in response");
  ESP_LOGI(TAG, "Network Scan GET Handler");
  ip_info_t device_info[NETWORK_PROVIDER_MAX] = {0};
  getDeviceIP(device_info);

  cJSON *response = cJSON_CreateArray();
  for (int i = 0; i < NETWORK_PROVIDER_MAX; i++) {
    if (!device_info[i].connected) {
      continue;
    }
    cJSON *response_network = cJSON_CreateObject();
    cJSON_AddStringToObject(response_network, "desc",
                            device_info[i].description);
    cJSON_AddStringToObject(response_network, "ip_addr",
                            device_info[i].ip_address);
    cJSON_AddStringToObject(response_network, "netmask",
                            device_info[i].netmask);
    cJSON_AddStringToObject(response_network, "gateway",
                            device_info[i].gateway);
    cJSON_AddStringToObject(response_network, "ssid", device_info[i].ssid);
    cJSON_AddItemToArray(response, response_network);
  }
  char *json_str = cJSON_PrintUnformatted(response);
  httpd_resp_send(req, json_str, strlen(json_str));
  vTaskDelay(500 / portTICK_PERIOD_MS);
  free(json_str);
  cJSON_Delete(response);
  return ESP_OK;
}

static const httpd_uri_t network_status = {.uri = "/network_status",
                                           .method = HTTP_GET,
                                           .handler = network_get_handler,
                                           .user_ctx = NULL};

httpd_handle_t start_webserver(void) {
  httpd_handle_t server = NULL;
  httpd_config_t config = HTTPD_DEFAULT_CONFIG();

  // Start the httpd server
  ESP_LOGI(TAG, "Starting server on port: '%d'", config.server_port);
  if (httpd_start(&server, &config) == ESP_OK) {
    // Set URI handlers
    ESP_LOGI(TAG, "Registering URI handlers");
    httpd_register_uri_handler(server, &wifi);
    httpd_register_uri_handler(server, &scan);
    httpd_register_uri_handler(server, &network_status);
#ifdef CONFIG_ENABLE_NETWORK_UI
    httpd_register_uri_handler(server, &network);
#endif
#ifdef CONFIG_ENABLE_FIRMWARE_UPLOAD_UI
    httpd_register_uri_handler(server, &ota_ui);
#endif
#ifdef CONFIG_ENABLE_OTA
    httpd_register_uri_handler(server, &ota);
#endif
    return server;
  }

  ESP_LOGI(TAG, "Error starting server!");
  return NULL;
}

void stop_webserver(httpd_handle_t server) {
  // Stop the httpd server
  httpd_stop(server);
}