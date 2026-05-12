
#ifndef _tcp_server_h_
#define _tcp_server_h_
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
extern int SocketPORT;
extern int tcp_socket;

extern uint8_t is_socket_connected;

extern TaskHandle_t Tcp_Server_Handle;
extern int listen_sock;
extern char addr_str[128];
extern int addr_family;
extern int ip_protocol;

void stop_tcp_server();
void tcp_server_task(void *pvParameters);
void initialize_tcp_socket();
void tcpsocket_configure(int online_sockets, char *ssid, int len);

#endif