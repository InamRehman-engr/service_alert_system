/* BSD Socket API Example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/
#include "esp_event.h"
#include "esp_log.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "freertos/task.h"
#include "nvs_flash.h"
#include <string.h>
#include <sys/param.h>

#include "connectivity.h"
#include "lwip/err.h"
#include "lwip/sockets.h"
#include "lwip/sys.h"
#include "tcp_server.h"
#include "vhmi.h"
#include <lwip/netdb.h>

#define DebugPrints
#define PRINT

#if defined(DebugPrints) && defined(PRINT)
#define print_d printf
#define printd_HEXDUMP ESP_LOG_BUFFER_HEXDUMP
#else
#define printd(...)
#define printd_HEXDUMP(...)
#endif

TaskHandle_t Tcp_Server_Handle = NULL;
uint8_t is_socket_connected = 0;
void Socket_online_status(uint8_t status);

void tcp_server_task(void *pvParameters) {
  char rx_buffer[128];

  waitDeviceHasIP();

  while (1) {

    int err = listen(listen_sock, 1);
    if (err != 0) {
      ESP_LOGE("socket", "Error occured during listen: errno %d", err);
      Socket_online_status(0);
      break;
    } else {
      ESP_LOGI("socket", "Socket listening");
      Socket_online_status(1);
      is_socket_connected = 1;
    }

    struct sockaddr_in6 sourceAddr; // Large enough for both IPv4 or IPv6
    uint32_t addrLen = sizeof(sourceAddr);
    tcp_socket = accept(listen_sock, (struct sockaddr *)&sourceAddr, &addrLen);
    if (tcp_socket < 0) {
      ESP_LOGE("socket", "Unable to accept connection");
      Socket_online_status(0);
      is_socket_connected = 0;
      break;
    }
    ESP_LOGI("socket", "Socket accepted");

    while (1) {

      int len = recv(tcp_socket, rx_buffer, sizeof(rx_buffer) - 1, 0);
      // Error occured during receiving
      if (len < 0) {
        //  ESP_LOGE("socket", "recv failed: errno %d", errno);
        break;
      }
      // Connection closed
      else if (len == 0) {
        ESP_LOGI("socket", "Connection closed");
        Socket_online_status(0);
        is_socket_connected = 0;
        break;
      }
      // Data received
      else {

        // Get the sender's ip address as string
        if (sourceAddr.sin6_family == PF_INET) {
          inet_ntoa_r(((struct sockaddr_in *)&sourceAddr)->sin_addr.s_addr,
                      addr_str, sizeof(addr_str) - 1);
        } else if (sourceAddr.sin6_family == PF_INET6) {
          inet6_ntoa_r(sourceAddr.sin6_addr, addr_str, sizeof(addr_str) - 1);
        }

        rx_buffer[len] =
            0; // Null-terminate whatever we received and treat like a string
        ESP_LOGI("socket", "Received %d bytes from %s:", len, addr_str);
        ESP_LOGI("socket", "%s", rx_buffer);

        data_recived_socket(
            len, (uint8_t *)rx_buffer); // data parsing at vhmi protocol

        // int err = send(tcp_socket, rx_buffer, len, 0);
        // if (err < 0) {
        //     ESP_LOGE(TAG, "Error occured during sending: errno %d", errno);
        //     break;
        // }
      }
    }
    break;
    //   if (tcp_socket != -1) {
    //     ESP_LOGE("socket", "Shutting down socket and restarting...");

    // if( Tcp_Server_Handle != NULL )
    // {   printf("\nin stop server\n");

    //     vTaskDelete(Tcp_Server_Handle);
    // }
    //         shutdown(tcp_socket, 0);
    //         close(tcp_socket);

    //     /* shutdown(tcp_socket, 0);
    //     close(tcp_socket); */
    // }
  }

  vTaskDelete(NULL);
}

void stop_tcp_server() {
  if (is_socket_connected) {
    Socket_online_status(0);

    if (Tcp_Server_Handle != NULL) {
      printf("\nin stop server\n");
      is_socket_connected = 0;

      vTaskDelete(Tcp_Server_Handle);
    }
    shutdown(tcp_socket, 0);
    close(tcp_socket);
  }
}

void initialize_tcp_socket() {

  waitDeviceHasIP();

#ifdef CONFIG_EXAMPLE_IPV4
  struct sockaddr_in destAddr;
  destAddr.sin_addr.s_addr = htonl(INADDR_ANY);
  destAddr.sin_family = AF_INET;
  destAddr.sin_port = htons(SocketPORT);
  addr_family = AF_INET;
  ip_protocol = IPPROTO_IP;
  inet_ntoa_r(destAddr.sin_addr, addr_str, sizeof(addr_str) - 1);
#else // IPV6
  struct sockaddr_in6 destAddr;
  bzero(&destAddr.sin6_addr.un, sizeof(destAddr.sin6_addr.un));
  destAddr.sin6_family = AF_INET6;
  destAddr.sin6_port = htons(SocketPORT);
  addr_family = AF_INET6;
  ip_protocol = IPPROTO_IPV6;
  inet6_ntoa_r(destAddr.sin6_addr, addr_str, sizeof(addr_str) - 1);
#endif

  listen_sock = socket(addr_family, SOCK_STREAM, ip_protocol);
  if (listen_sock < 0) {
    ESP_LOGE("socket", "Unable to create socket");
    //   break;
  }
  ESP_LOGI("socket", "Socket created");

  int err = bind(listen_sock, (struct sockaddr *)&destAddr, sizeof(destAddr));
  if (err != 0) {
    ESP_LOGE("socket", "Socket unable to bind: errno %d", err);
    //    PORT=5552;
    vTaskDelay(200);

    //   continue;
  }
  ESP_LOGI("socket", "Socket binded");
}

esp_err_t tcpsocket_publish_vhmi_data(uint16_t size, uint8_t *data) {
  if (tcp_socket > 0) {
    int err = send(tcp_socket, (uint8_t *)data, size, 0);
    if (err < 0) {
      ESP_LOGE("sockets", "Error occured during sending: errno %d", err);
    } else {
      ESP_LOGD("sockets", "\nData send on socket successfuly\n");
      return ESP_OK;
    }
  } else {
    ESP_LOGE("sockets", "\nSocket not opened\n");
  }
  return ESP_FAIL;
}

void Socket_online_status(uint8_t status) {
  vhmi_cmd_t msg;
  msg.cmd_type = vhmi_cmd_type_socket_status;

  if (status == 1) {

    msg.Socket_data.status = (uint8_t)status;
    msg.Socket_data.port = SocketPORT;
    memcpy(&msg.Socket_data.ip, &wifi_ip_info.ip, sizeof(ip4_addr_t));
    print_d("IP = " IPSTR "\n\n", IP2STR((ip4_addr_t *)&msg.Socket_data.ip));
    print_d("PORT = %d\n\n", msg.Socket_data.port);
    printd_HEXDUMP("IP address", &msg.Socket_data.ip, 4, LOG_LOCAL_LEVEL);
  } else {
    msg.Socket_data.status = (uint8_t)status;
    msg.Socket_data.port = 0;
    msg.Socket_data.ip = 0;
  }
  msg.len = 9;
  send_data_on_vhmi_channel(&msg);
}

void tcpsocket_configure(int online_sockets, char *ssid, int len) {
  // print_d("\nsifi_ssid = %s\n",*wifi_ssid);

  if (online_sockets == 1 && ssid != NULL) {

    if (strncmp(wifiSSID, ssid, len) == 0) {

      xTaskCreate(tcp_server_task, "tcp_server", 4096, NULL, 5,
                  &Tcp_Server_Handle);
      //  vTaskDelay(2000 / portTICK_PERIOD_MS);
      // Socket_online_status(0);
    } else {
      Socket_online_status(0);
    }
  } else {
    ESP_LOGW("Socket", "Shutting down socket");
    Socket_online_status(0);
    stop_tcp_server();
  }
}