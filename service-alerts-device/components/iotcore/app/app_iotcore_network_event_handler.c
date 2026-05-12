#include "app_iotcore_network_event_handler.h"

#include "iotcore_events.h"

#include "string.h"

#include "esp_netif.h"
#include "esp_wifi.h"

esp_netif_ip_info_t ethernet_ip_info;
esp_netif_ip_info_t ppp_ip_info;
esp_ip4_addr_t wifi_ip;
char wifi_ssid[33];
void network_status_event_handler(void *event_handler_arg,
                                  esp_event_base_t event_base, int32_t event_id,
                                  void *event_data) {
  char *message = NULL;
  char *topic = NULL;
  !event_handler_arg ? ({ return; }) : 0;
  switch (event_id) {
  case IP_EVENT_PPP_GOT_IP:
#if defined(CONFIG_ENABLE_MODEM) || (CONFIG_ENABLE_EPPP_CLIENT)
  {
    ip_event_got_ip_t *event = (ip_event_got_ip_t *)event_data;
    const esp_netif_ip_info_t *ip_info = &event->ip_info;

    if (memcmp(&ppp_ip_info, ip_info, sizeof(esp_netif_ip_info_t)) != 0) {
      asiprintf(&message,
                "{\"ip\":\"" IPSTR "\",\"mask\":\"" IPSTR
                "\",\"gateway\":\"" IPSTR "\"}",
                IP2STR(&ip_info->ip), IP2STR(&ip_info->netmask),
                IP2STR(&ip_info->gw));
      asiprintf(&topic, "d/%ld/status/network/ppp",
                *(int32_t *)event_handler_arg);
      memcpy(&ppp_ip_info, ip_info, sizeof(esp_netif_ip_info_t));
    } else {
      return;
    }
    break;
  case IP_EVENT_PPP_LOST_IP:
    asiprintf(&message, "{}");
    asiprintf(&topic, "d/%ld/status/network/ppp",
              *(int32_t *)event_handler_arg);

    memset(&ppp_ip_info, 0, sizeof(esp_netif_ip_info_t));
  }
#endif
  break;
  case IP_EVENT_ETH_GOT_IP:
#ifdef CONFIG_ENABLE_ETHERNET
  {
    ip_event_got_ip_t *event = (ip_event_got_ip_t *)event_data;
    const esp_netif_ip_info_t *ip_info = &event->ip_info;
    if (memcmp(&ethernet_ip_info, ip_info, sizeof(esp_netif_ip_info_t)) != 0) {
      asiprintf(&message,
                "{\"ip\":\"" IPSTR "\",\"mask\":\"" IPSTR
                "\":\"gateway\":" IPSTR "}",
                IP2STR(&ip_info->ip), IP2STR(&ip_info->netmask),
                IP2STR(&ip_info->gw));
      asiprintf(&topic, "d/%ld/status/network/eth",
                *(int32_t *)event_handler_arg);
      memcpy(&ethernet_ip_info, ip_info, sizeof(esp_netif_ip_info_t));
    } else {
      return;
    }
    break;
  case IP_EVENT_ETH_LOST_IP:
    asiprintf(&message, "{}");
    asiprintf(&topic, "d/%ld/status/network/eth",
              *(int32_t *)event_handler_arg);
    memset(&ethernet_ip_info, 0, sizeof(esp_netif_ip_info_t));
  }
#endif
  break;
  case IP_EVENT_STA_GOT_IP:
#ifdef CONFIG_ENABLE_WIFI
  {
    const ip_event_got_ip_t *got_ip = (ip_event_got_ip_t *)event_data;
    wifi_ap_record_t ap_info;
    esp_wifi_sta_get_ap_info(&ap_info);
    if (memcmp(&wifi_ip, &got_ip->ip_info.ip, sizeof(esp_ip4_addr_t)) != 0 ||
        strncmp(wifi_ssid, (char *)ap_info.ssid, 33) != 0) {
      asiprintf(&message, "{\"ssid\":\"%s\",\"ip\":\"" IPSTR "\":\"ch\":%d}",
                (char *)ap_info.ssid, IP2STR(&got_ip->ip_info.ip),
                ap_info.primary);
      asiprintf(&topic, "d/%ld/status/network/wifi",
                *(int32_t *)event_handler_arg);
      memcpy(&wifi_ip, &got_ip->ip_info.ip, sizeof(esp_ip4_addr_t));
      strncpy(wifi_ssid, (char *)ap_info.ssid, 33);
    } else {
      return;
    }
    break;
  case IP_EVENT_STA_LOST_IP:
    asiprintf(&message, "{}");
    asiprintf(&topic, "d/%ld/status/network/wifi",
              *(int32_t *)event_handler_arg);
    memset(&wifi_ip, 0, sizeof(esp_ip4_addr_t));
  }
#endif
  break;
  default:
    break;
  }

  message ? post_mqtt_on_connect_publish_event(message, strlen(message), topic,
                                               true, 2)
          : 0;
  topic ? free(topic) : 0;
  message ? free(message) : 0;
}

void init_network_event_handler(int32_t *clientID) {
  memset(&ethernet_ip_info, 0, sizeof(esp_netif_ip_info_t));
  memset(&wifi_ip, 0, sizeof(esp_ip4_addr_t));
#if defined(CONFIG_ENABLE_MODEM) || (CONFIG_ENABLE_EPPP_CLIENT)
  esp_event_handler_register(IP_EVENT, IP_EVENT_PPP_GOT_IP,
                             &network_status_event_handler, clientID);
  esp_event_handler_register(IP_EVENT, IP_EVENT_PPP_LOST_IP,
                             &network_status_event_handler, clientID);
#endif
#ifdef CONFIG_ENABLE_ETHERNET
  esp_event_handler_register(IP_EVENT, IP_EVENT_ETH_GOT_IP,
                             &network_status_event_handler, clientID);
  esp_event_handler_register(IP_EVENT, IP_EVENT_ETH_LOST_IP,
                             &network_status_event_handler, clientID);
#endif
#ifdef CONFIG_ENABLE_WIFI
  esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP,
                             &network_status_event_handler, clientID);
  esp_event_handler_register(IP_EVENT, IP_EVENT_STA_LOST_IP,
                             &network_status_event_handler, clientID);
#endif
}