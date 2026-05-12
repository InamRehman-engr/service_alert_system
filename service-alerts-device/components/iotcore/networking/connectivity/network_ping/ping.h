/* ICMP echo example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/

#ifndef PING_H
#define PING_H

#include "esp_event.h"
#include "lwip/inet.h"
#include "lwip/netdb.h"
#include "lwip/sockets.h"
#include "ping/ping_sock.h"
#include "sdkconfig.h"
#include <stdio.h>
#include <string.h>

esp_ping_handle_t do_ping_cmd(
    char *address, int netif_index, int has_internet_bit,
    void (*fun_ptr)(
        bool state,
        int bit)); // Give IP preferrably for address enumeration requires
                   // active internet. to start at app init give ip
void delete_ping_cmd(esp_ping_handle_t *ping);

#endif // PING_H