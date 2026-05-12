#include "connectivity.h"
#include "connectivity_overrides.h"

#include "esp_bit_defs.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_netif.h"
#include "esp_netif_ppp.h"
#include "esp_system.h"
#include "esp_timer.h"
#include "esp_wifi.h"
#include "freertos/event_groups.h"
#include <stddef.h>

#include "ping.h"
/// TODO: Optimize modem recovery time
#define DNS_SERVER_ADDRESS                                                     \
  CONFIG_PING_IP_ADDRESS // Example: Google's DNS server
#define MAX_PING_FAILURES CONFIG_MAX_PING_FAILURES
uint8_t pingFailures = 0;

bool custom_internet_check_proder = false;

int64_t last_internet_time = 0;
EventGroupHandle_t s_connectivity_event_group =
    NULL; // This will be initialized in connectivity init. don't use before

uint8_t default_priority[NETWORK_PROVIDER_MAX] = {
    NETWORK_PROVIDER_WIFI, NETWORK_PROVIDER_NONE, NETWORK_PROVIDER_NONE,
    NETWORK_PROVIDER_NONE};

bool network_started[NETWORK_PROVIDER_MAX] = {false};
bool ping_started[NETWORK_PROVIDER_MAX] = {false};
esp_ping_handle_t ping_handles[NETWORK_PROVIDER_MAX] = {NULL};
typedef struct {
  esp_netif_t *netif_handle;
  esp_netif_ip_info_t ip_info;
} connection_info_t;
connection_info_t device_connection_info[NETWORK_PROVIDER_MAX] = {0};

uint8_t user_priority[NETWORK_PROVIDER_MAX];
enum connectivity_network_provider_t current_provider = NETWORK_PROVIDER_NONE;
int WIFI_CONNECTED_BIT = BIT0; // Wi-Fi connection established
int WIFI_HAS_IP_BIT = BIT1;    // Device has obtained an IP address over Wi-Fi
int WIFI_HAS_INTERNET_BIT = BIT2;  // Internet access is available over Wi-Fi
int GSM_HAS_SIM_CARD_BIT = BIT3;   // GSM module detects a SIM card
int GSM_CONNECTED_BIT = BIT4;      // GSM connection established
int GSM_HAS_IP_BIT = BIT5;         // Device has obtained an IP address over GSM
int GSM_HAS_INTERNET_BIT = BIT6;   // Internet access is available over GSM
int ETHERNET_CONNECTED_BIT = BIT7; // Ethernet connection established
int ETHERNET_HAS_IP_BIT =
    BIT8; // Device has obtained an IP address over Ethernet
int ETHERNET_HAS_INTERNET_BIT =
    BIT9;                       // Internet access is available over Ethernet
int EPPP_CONNECTED_BIT = BIT10; // EPPP connection established
int EPPP_HAS_IP_BIT = BIT11;    // Device has obtained an IP address over EPPP
int EPPP_HAS_INTERNET_BIT = BIT12; // Internet access if available over EPPP
int DEVICE_HAS_INTERNET = BIT23;   // Device has internet access      (BIT23 is
                                   // final bit of xEventGroup created)

static const char *TAG = "Connectivity";

#ifdef ALLOW_CUSTOM_INTERNET_AVAILABILITY_CHECK
void connectivity_override_internet_availability(bool available) {
  available
      ? xEventGroupSetBits(s_connectivity_event_group, DEVICE_HAS_INTERNET)
      : xEventGroupClearBits(s_connectivity_event_group, DEVICE_HAS_INTERNET);
#if defined(CONFIG_ENABLE_WIFI)
  available
      ? xEventGroupSetBits(s_connectivity_event_group, WIFI_HAS_INTERNET_BIT)
      : xEventGroupClearBits(s_connectivity_event_group, WIFI_HAS_INTERNET_BIT);
#elif defined(CONFIG_ENABLE_ETHERNET)
  available ? xEventGroupSetBits(s_connectivity_event_group,
                                 ETHERNET_HAS_INTERNET_BIT)
            : xEventGroupClearBits(s_connectivity_event_group,
                                   ETHERNET_HAS_INTERNET_BIT);
#elif defined(CONFIG_ENABLE_MODEM)
  available
      ? xEventGroupSetBits(s_connectivity_event_group, GSM_HAS_INTERNET_BIT)
      : xEventGroupClearBits(s_connectivity_event_group, GSM_HAS_INTERNET_BIT);
#elif defined(CONFIG_ENABLE_EPPP_CLIENT)
  available
      ? xEventGroupSetBits(s_connectivity_event_group, EPPP_HAS_INTERNET_BIT)
      : xEventGroupClearBits(s_connectivity_event_group, EPPP_HAS_INTERNET_BIT);
#endif
}
#endif

void stop_interface(uint8_t priority) {
  switch (user_priority[priority]) {
  case NETWORK_PROVIDER_WIFI:
#ifdef CONFIG_ENABLE_WIFI
    if (network_started[NETWORK_PROVIDER_WIFI]) {
      ESP_LOGW(TAG, "Stopping Wifi");
      stop_wifi_manager();
      network_started[NETWORK_PROVIDER_WIFI] = 0;
    }
#endif
    break;
  case NETWORK_PROVIDER_ETHERNET:
#ifdef CONFIG_ENABLE_ETHERNET
    if (network_started[NETWORK_PROVIDER_ETHERNET]) {
      ESP_LOGW(TAG, "Stopping Ethernet");
      !custom_internet_check_proder ? ({
        if (ping_started[NETWORK_PROVIDER_ETHERNET]) {
          delete_ping_cmd(ping_handles[NETWORK_PROVIDER_ETHERNET]);
          xEventGroupClearBits(s_connectivity_event_group,
                               ETHERNET_HAS_INTERNET_BIT);
          ping_started[NETWORK_PROVIDER_ETHERNET] = false;
        }
      })
                                    : ({});
      ethernet_stop();
      network_started[NETWORK_PROVIDER_ETHERNET] = 0;
    }
#endif
    break;
  case NETWORK_PROVIDER_MODEM:
#ifdef CONFIG_ENABLE_MODEM
    setModemDataState(ESP_MODEM_MODE_COMMAND);
    network_started[NETWORK_PROVIDER_MODEM] = 0;
#endif
    break;
  case NETWORK_PROVIDER_EPPP_LINK:
#ifdef CONFIG_ENABLE_EPPP_CLIENT
    if (network_started[NETWORK_PROVIDER_EPPP_LINK]) {
      ESP_LOGW(TAG, "Stopping EPPP Client");
      eppp_client_stop();
      !custom_internet_check_proder ? ({
        if (ping_started[NETWORK_PROVIDER_EPPP_LINK]) {
          delete_ping_cmd(ping_handles[NETWORK_PROVIDER_EPPP_LINK]);
          xEventGroupClearBits(s_connectivity_event_group,
                               EPPP_HAS_INTERNET_BIT);
          ping_started[NETWORK_PROVIDER_EPPP_LINK] = false;
        }
      })
                                    : ({});
      network_started[NETWORK_PROVIDER_EPPP_LINK] = 0;
    }
#endif
    break;
  default:
    break;
  }
}

void start_interface(uint8_t priority) {
  switch (user_priority[priority]) {
  case NETWORK_PROVIDER_WIFI:
#ifdef CONFIG_ENABLE_WIFI
    if (network_started[NETWORK_PROVIDER_WIFI] == 0) {
      ESP_LOGW(TAG, "Starting Wifi");
      start_wifi_manager();
      network_started[NETWORK_PROVIDER_WIFI] = 1;
    }
#endif
    break;
  case NETWORK_PROVIDER_ETHERNET:
#ifdef CONFIG_ENABLE_ETHERNET
    if (network_started[NETWORK_PROVIDER_ETHERNET] == 0) {
      ESP_LOGW(TAG, "Starting Ethernet");
      if (ethernet_start() == ESP_OK)
        network_started[NETWORK_PROVIDER_ETHERNET] = 1;
    }
#endif
    break;
  case NETWORK_PROVIDER_MODEM:
#ifdef CONFIG_ENABLE_MODEM
    setModemDataState(ESP_MODEM_MODE_CMUX);
    network_started[NETWORK_PROVIDER_MODEM] = 1;
#endif
    break;
  case NETWORK_PROVIDER_EPPP_LINK:
#ifdef CONFIG_ENABLE_EPPP_CLIENT
    if (network_started[NETWORK_PROVIDER_EPPP_LINK] == 0) {
      eppp_client_start();
      network_started[NETWORK_PROVIDER_EPPP_LINK] = 1;
    }
#endif
    break;
  default:
    break;
  }
}

bool check_if_this_if_has_internet(
    uint8_t priority) { // Call this after the interface reports an IP address
  int check = false;
  switch (user_priority[priority]) {
  case NETWORK_PROVIDER_WIFI:
#ifdef CONFIG_ENABLE_WIFI
    check = xEventGroupGetBits(s_connectivity_event_group) &
            (WIFI_HAS_INTERNET_BIT);
    ESP_LOGD(TAG, "Wifi has Internet check");
    if ((xEventGroupGetBits(s_connectivity_event_group) &
         (WIFI_HAS_INTERNET_BIT)) == 0) {
      pingFailures++;
      if (pingFailures >= MAX_PING_FAILURES) {
        xEventGroupClearBits(s_connectivity_event_group, WIFI_HAS_INTERNET_BIT);
        rotateWifiList();
        ESP_LOGD(TAG, "PRIORITY ADJUSTED");
        pingFailures = 0;
      }

      ESP_LOGD(TAG, "PING FAILURES: %d", pingFailures);
    } else {
      pingFailures = 0;
    }
#endif
    break;
  case NETWORK_PROVIDER_ETHERNET:
#ifdef CONFIG_ENABLE_ETHERNET
    check = xEventGroupGetBits(s_connectivity_event_group) &
            (ETHERNET_HAS_INTERNET_BIT);
    ESP_LOGD(TAG, "Ethernet has Internet check");
#endif
    break;
  case NETWORK_PROVIDER_MODEM:
#ifdef CONFIG_ENABLE_MODEM
    check =
        xEventGroupGetBits(s_connectivity_event_group) & (GSM_HAS_INTERNET_BIT);
    ESP_LOGD(TAG, "Modem has Internet check");
#endif
    break;
  case NETWORK_PROVIDER_EPPP_LINK:
#ifdef CONFIG_EPPP_CLIENT
    check = xEventGroupGetBits(s_connectivity_event_group) &
            (EPPP_HAS_INTERNET_BIT);
    ESP_LOGD(TAG, "EPPP has Internet check");
#endif
    break;
  default:
    break;
  }
  return check;
}
#ifdef CONFIG_ENABLE_MODEM
static void on_ppp_changed(void *arg, esp_event_base_t event_base,
                           int32_t event_id, void *event_data) {
  ESP_LOGI(TAG, "PPP state changed event %" PRIu32, event_id);
  if (event_id == NETIF_PPP_ERRORUSER) {
    /* User interrupted event from esp-netif */
    esp_netif_t *netif = (esp_netif_t *)event_data;
    ESP_LOGI(TAG, "User interrupted event from netif:%p", netif);
    xEventGroupClearBits(s_connectivity_event_group, GSM_HAS_IP_BIT);
    // For now this is indication of internet availability
    xEventGroupClearBits(s_connectivity_event_group, GSM_HAS_INTERNET_BIT);
    !custom_internet_check_proder ? ({
      if (ping_started[NETWORK_PROVIDER_MODEM]) {
        delete_ping_cmd(ping_handles[NETWORK_PROVIDER_MODEM]);
        ping_started[NETWORK_PROVIDER_MODEM] = false;
      }
    })
                                  : ({});
  }
}
#endif

void network_state_change_func(bool state, int bit);

static void on_ip_event(void *arg, esp_event_base_t event_base,
                        int32_t event_id, void *event_data) {
  ESP_LOGD(TAG, "IP event! %" PRIu32, event_id);
#ifdef CONFIG_ENABLE_MODEM
  if (event_id == IP_EVENT_PPP_GOT_IP) {
    esp_netif_dns_info_t dns_info;

    ip_event_got_ip_t *event = (ip_event_got_ip_t *)event_data;
    device_connection_info[NETWORK_PROVIDER_MODEM].netif_handle =
        event->esp_netif;
    device_connection_info[NETWORK_PROVIDER_MODEM].ip_info = event->ip_info;
    esp_netif_t *netif = event->esp_netif;

    ESP_LOGD(TAG, "Modem Connect to PPP Server");
    ESP_LOGD(TAG, "~~~~~~~~~~~~~~");
    ESP_LOGI(TAG, "IP          : " IPSTR, IP2STR(&event->ip_info.ip));
    ESP_LOGD(TAG, "Netmask     : " IPSTR, IP2STR(&event->ip_info.netmask));
    ESP_LOGD(TAG, "Gateway     : " IPSTR, IP2STR(&event->ip_info.gw));
    esp_netif_get_dns_info(netif, ESP_NETIF_DNS_MAIN, &dns_info);
    ESP_LOGD(TAG, "Name Server1: " IPSTR, IP2STR(&dns_info.ip.u_addr.ip4));
    esp_netif_get_dns_info(netif, ESP_NETIF_DNS_BACKUP, &dns_info);
    ESP_LOGD(TAG, "Name Server2: " IPSTR, IP2STR(&dns_info.ip.u_addr.ip4));
    ESP_LOGD(TAG, "~~~~~~~~~~~~~~");
    xEventGroupSetBits(s_connectivity_event_group, GSM_HAS_IP_BIT);
    // For now this is indication of internet availability
    xEventGroupSetBits(s_connectivity_event_group, GSM_HAS_INTERNET_BIT);

    ESP_LOGD(TAG, "GOT ip event!!!");
    !custom_internet_check_proder ? ({
      if (ping_started[NETWORK_PROVIDER_MODEM]) {
        delete_ping_cmd(ping_handles[NETWORK_PROVIDER_MODEM]);
        xEventGroupClearBits(s_connectivity_event_group, GSM_HAS_INTERNET_BIT);
        ping_started[NETWORK_PROVIDER_MODEM] = false;
      }
      const char *if_key = esp_netif_get_ifkey(event->esp_netif);
      esp_netif_t *modem_netif = esp_netif_get_handle_from_ifkey(if_key);
      if (modem_netif != NULL) {
        int index = esp_netif_get_netif_impl_index(modem_netif);
        if (index != -1) {
          if (!ping_started[NETWORK_PROVIDER_MODEM]) {
            ping_handles[NETWORK_PROVIDER_MODEM] =
                do_ping_cmd(DNS_SERVER_ADDRESS, index, GSM_HAS_INTERNET_BIT,
                            network_state_change_func);
            ping_started[NETWORK_PROVIDER_MODEM] = true;
          }
        }
      }
    })
                                  : ({});
  }

  if (event_id == IP_EVENT_PPP_LOST_IP) {
    ESP_LOGI(TAG, "Modem Disconnect from PPP Server");
    device_connection_info[NETWORK_PROVIDER_MODEM].netif_handle = NULL;
    memset(&device_connection_info[NETWORK_PROVIDER_MODEM].ip_info, 0,
           sizeof(esp_netif_ip_info_t));
    xEventGroupClearBits(s_connectivity_event_group, GSM_HAS_IP_BIT);
    // For now this is indication of internet availability
    xEventGroupClearBits(s_connectivity_event_group, GSM_HAS_INTERNET_BIT);
    !custom_internet_check_proder ? ({
      if (ping_started[NETWORK_PROVIDER_MODEM]) {
        delete_ping_cmd(ping_handles[NETWORK_PROVIDER_MODEM]);
        ping_started[NETWORK_PROVIDER_MODEM] = false;
      }
    })
                                  : ({});
  }
#endif

#ifdef CONFIG_ENABLE_EPPP_CLIENT
  if (event_id == IP_EVENT_PPP_GOT_IP) {
    esp_netif_dns_info_t dns_info;

    ip_event_got_ip_t *event = (ip_event_got_ip_t *)event_data;
    device_connection_info[NETWORK_PROVIDER_EPPP_LINK].netif_handle =
        event->esp_netif;
    device_connection_info[NETWORK_PROVIDER_EPPP_LINK].ip_info = event->ip_info;
    esp_netif_t *netif = event->esp_netif;

    ESP_LOGD(TAG, "EPPP Link Connect to PPP Server");
    ESP_LOGD(TAG, "~~~~~~~~~~~~~~");
    ESP_LOGI(TAG, "IP          : " IPSTR, IP2STR(&event->ip_info.ip));
    ESP_LOGD(TAG, "Netmask     : " IPSTR, IP2STR(&event->ip_info.netmask));
    ESP_LOGD(TAG, "Gateway     : " IPSTR, IP2STR(&event->ip_info.gw));
    esp_netif_get_dns_info(netif, ESP_NETIF_DNS_MAIN, &dns_info);
    ESP_LOGD(TAG, "Name Server1: " IPSTR, IP2STR(&dns_info.ip.u_addr.ip4));
    esp_netif_get_dns_info(netif, ESP_NETIF_DNS_BACKUP, &dns_info);
    ESP_LOGD(TAG, "Name Server2: " IPSTR, IP2STR(&dns_info.ip.u_addr.ip4));
    ESP_LOGD(TAG, "~~~~~~~~~~~~~~");
    xEventGroupSetBits(s_connectivity_event_group, EPPP_HAS_IP_BIT);

    ESP_LOGD(TAG, "GOT ip event!!!");
    !custom_internet_check_proder ? ({
      if (ping_started[NETWORK_PROVIDER_EPPP_LINK]) {
        delete_ping_cmd(ping_handles[NETWORK_PROVIDER_EPPP_LINK]);
        xEventGroupClearBits(s_connectivity_event_group, EPPP_HAS_INTERNET_BIT);
        ping_started[NETWORK_PROVIDER_EPPP_LINK] = false;
      }
      const char *if_key = esp_netif_get_ifkey(event->esp_netif);
      esp_netif_t *eppp_netif = esp_netif_get_handle_from_ifkey(if_key);
      if (eppp_netif != NULL) {
        int index = esp_netif_get_netif_impl_index(eppp_netif);
        if (index != -1) {
          if (!ping_started[NETWORK_PROVIDER_EPPP_LINK]) {
            ping_handles[NETWORK_PROVIDER_EPPP_LINK] =
                do_ping_cmd(DNS_SERVER_ADDRESS, index, EPPP_HAS_INTERNET_BIT,
                            network_state_change_func);
            ping_started[NETWORK_PROVIDER_EPPP_LINK] = true;
          }
        }
      }
    })
                                  : ({});
  }

  if (event_id == IP_EVENT_PPP_LOST_IP) {
    ESP_LOGI(TAG, "EPPP Disconnect from PPP Server");
    device_connection_info[NETWORK_PROVIDER_EPPP_LINK].netif_handle = NULL;
    memset(&device_connection_info[NETWORK_PROVIDER_EPPP_LINK].ip_info, 0,
           sizeof(esp_netif_ip_info_t));
    !custom_internet_check_proder ? ({
      if (ping_started[NETWORK_PROVIDER_EPPP_LINK]) {
        delete_ping_cmd(ping_handles[NETWORK_PROVIDER_EPPP_LINK]);
        ping_started[NETWORK_PROVIDER_EPPP_LINK] = false;
      }
      xEventGroupClearBits(s_connectivity_event_group, EPPP_HAS_IP_BIT);
      xEventGroupClearBits(s_connectivity_event_group, EPPP_HAS_INTERNET_BIT);
    })
                                  : ({});
  }
#endif

#ifdef CONFIG_ENABLE_ETHERNET
  if (event_id == IP_EVENT_ETH_GOT_IP) {
    xEventGroupSetBits(s_connectivity_event_group, ETHERNET_HAS_IP_BIT);
    ip_event_got_ip_t *event = (ip_event_got_ip_t *)event_data;
    const esp_netif_ip_info_t *ip_info = &event->ip_info;
    device_connection_info[NETWORK_PROVIDER_ETHERNET].netif_handle =
        event->esp_netif;
    device_connection_info[NETWORK_PROVIDER_ETHERNET].ip_info = event->ip_info;

    ESP_LOGD(TAG, "Ethernet Got IP Address");
    ESP_LOGD(TAG, "~~~~~~~~~~~");
    ESP_LOGI(TAG, "ETHIP:" IPSTR, IP2STR(&ip_info->ip));
    ESP_LOGD(TAG, "ETHMASK:" IPSTR, IP2STR(&ip_info->netmask));
    ESP_LOGD(TAG, "ETHGW:" IPSTR, IP2STR(&ip_info->gw));
    ESP_LOGD(TAG, "~~~~~~~~~~~");

    xEventGroupSetBits(s_connectivity_event_group, ETHERNET_HAS_IP_BIT);
    !custom_internet_check_proder ? ({
      if (ping_started[NETWORK_PROVIDER_ETHERNET]) {
        delete_ping_cmd(ping_handles[NETWORK_PROVIDER_ETHERNET]);
        xEventGroupClearBits(s_connectivity_event_group,
                             ETHERNET_HAS_INTERNET_BIT);
        ping_started[NETWORK_PROVIDER_ETHERNET] = false;
      }

      const char *if_key = esp_netif_get_ifkey(event->esp_netif);
      esp_netif_t *ethernet_netif = esp_netif_get_handle_from_ifkey(if_key);

      if (ethernet_netif != NULL) {
        int index = esp_netif_get_netif_impl_index(ethernet_netif);

        if (index != -1) {
          if (!ping_started[NETWORK_PROVIDER_ETHERNET]) {
            ping_handles[NETWORK_PROVIDER_ETHERNET] = do_ping_cmd(
                DNS_SERVER_ADDRESS, index, ETHERNET_HAS_INTERNET_BIT,
                network_state_change_func);
            ping_started[NETWORK_PROVIDER_ETHERNET] = true;
          }
        }
      }
    })
                                  : ({});
  }

  if (event_id == IP_EVENT_ETH_LOST_IP) {
    ESP_LOGI(TAG, "Ethernet Lost IP Address");
    device_connection_info[NETWORK_PROVIDER_ETHERNET].netif_handle = NULL;
    memset(&device_connection_info[NETWORK_PROVIDER_ETHERNET].ip_info, 0,
           sizeof(esp_netif_ip_info_t));
    !custom_internet_check_proder ? ({
      if (ping_started[NETWORK_PROVIDER_ETHERNET]) {
        delete_ping_cmd(ping_handles[NETWORK_PROVIDER_ETHERNET]);
        xEventGroupClearBits(s_connectivity_event_group,
                             ETHERNET_HAS_INTERNET_BIT);
        ping_started[NETWORK_PROVIDER_ETHERNET] = false;
      }
      xEventGroupClearBits(s_connectivity_event_group, ETHERNET_HAS_IP_BIT);
      xEventGroupClearBits(s_connectivity_event_group,
                           ETHERNET_HAS_INTERNET_BIT);
    })
                                  : ({});
  }
#endif

  if (event_id == IP_EVENT_GOT_IP6) {
    ESP_LOGI(TAG, "GOT IPv6 event!");
    ip_event_got_ip6_t *event = (ip_event_got_ip6_t *)event_data;
    ESP_LOGI(TAG, "Got IPv6 address " IPV6STR, IPV62STR(event->ip6_info.ip));
  }
#ifdef CONFIG_ENABLE_WIFI

  if (event_id == IP_EVENT_STA_GOT_IP) {
    xEventGroupSetBits(s_connectivity_event_group, WIFI_HAS_IP_BIT);
    ip_event_got_ip_t *event = (ip_event_got_ip_t *)event_data;
    device_connection_info[NETWORK_PROVIDER_WIFI].netif_handle =
        event->esp_netif;
    device_connection_info[NETWORK_PROVIDER_WIFI].ip_info = event->ip_info;
    !custom_internet_check_proder ? ({
      if (ping_started[NETWORK_PROVIDER_WIFI]) {
        delete_ping_cmd(ping_handles[NETWORK_PROVIDER_WIFI]);
        ping_started[NETWORK_PROVIDER_WIFI] = false;
      }
      esp_netif_t *sta_netif = esp_netif_get_handle_from_ifkey("WIFI_STA_DEF");

      if (sta_netif != NULL) {
        int index = esp_netif_get_netif_impl_index(sta_netif);

        if (index != -1) {
          if (!ping_started[NETWORK_PROVIDER_WIFI]) {
            ping_handles[NETWORK_PROVIDER_WIFI] =
                do_ping_cmd(DNS_SERVER_ADDRESS, index, WIFI_HAS_INTERNET_BIT,
                            network_state_change_func);
            ping_started[NETWORK_PROVIDER_WIFI] = true;
          }
        }
      }
    })
                                  : ({});
  }

  if (event_id == IP_EVENT_STA_LOST_IP) {
    device_connection_info[NETWORK_PROVIDER_WIFI].netif_handle = NULL;
    memset(&device_connection_info[NETWORK_PROVIDER_WIFI].ip_info, 0,
           sizeof(esp_netif_ip_info_t));
    !custom_internet_check_proder ? ({
      if (ping_started[NETWORK_PROVIDER_WIFI]) {
        delete_ping_cmd(ping_handles[NETWORK_PROVIDER_WIFI]);
        ping_started[NETWORK_PROVIDER_WIFI] = false;
      }
    })
                                  : ({});
    xEventGroupClearBits(s_connectivity_event_group, WIFI_HAS_IP_BIT);
  }
#endif
}
/*Start Block*/
/*
    This block gives connectivity flag that indicates if internet has
   disconnected
    - On a stable wifi connection it never timeouts. But on modem it might
   timeout quite frequently
    - In future this might need to be done with retries
*/

void network_state_change_func(bool state, int bit) {
  /// TODO: Add tries count for interface.
  if ((bool)(xEventGroupGetBits(s_connectivity_event_group) & bit) != state)
    ESP_LOGW(TAG, "Internet availability state changed. Current state: %d",
             state);
  if (state) {
    xEventGroupSetBits(s_connectivity_event_group, bit);
  } else {
    xEventGroupClearBits(s_connectivity_event_group, bit);
  }
}
/*End Block*/

void connectivity_task(void *pvParams) {
  /*
  Some things we will need to assume.
      - User application side will provide priority. /// TODO: Decide how that
  will look like
      - We will give grace period to any interface before bringing in others to
  pick up the connection.
      - Higher priority will keep on trying to connect while lower priority is
  providing internet Tasks to be performed here.
      - Handle starting and stopping of IFs. (Wifi modem and ethernet)
      - Start fallback IFs if the primary cannot provide internet
      - Turn of lower priority one if the higher one is providing internet
  */
  // Provider Selection

  // xEventGroupWaitBits(s_connectivity_event_group, GSM_HAS_IP_BIT |
  // WIFI_HAS_IP_BIT | ETHERNET_HAS_IP_BIT, pdTRUE, pdTRUE, portMAX_DELAY);

  // Start New here
  while (1) {
    /// TODO: Figure out method for grace periods
    /// TODO: Make IFs have checks inthem for already init or not initialized
    int x = 0;
    for (; x < sizeof(user_priority); x++) {
      if (check_if_this_if_has_internet(x)) {
        break;
      }
      start_interface(
          x); /*  This is what will start the IF's. for the first time it will
                 probably enable an interface and not get internet access. It
                 will probably get that on the next iteration of the loop.*/
    }
    x++;
    for (; x < sizeof(user_priority); x++) {
      stop_interface(x); /*   Stop Interfaces that are not needed as a higher
                          priority one is providing internet access*/
    }

    if (((xEventGroupGetBits(s_connectivity_event_group) &
          WIFI_HAS_INTERNET_BIT) ||
         (xEventGroupGetBits(s_connectivity_event_group) &
          ETHERNET_HAS_INTERNET_BIT) ||
         (xEventGroupGetBits(s_connectivity_event_group) &
          GSM_HAS_INTERNET_BIT) ||
         (xEventGroupGetBits(s_connectivity_event_group) &
          EPPP_HAS_INTERNET_BIT)) != 0) {
      xEventGroupSetBits(s_connectivity_event_group, DEVICE_HAS_INTERNET);
      ESP_LOGD(TAG, "DEVICE HAS INTERNET!!!  %d   %d   %d   %d",
               (bool)(xEventGroupGetBits(s_connectivity_event_group) &
                      WIFI_HAS_INTERNET_BIT),
               (bool)(xEventGroupGetBits(s_connectivity_event_group) &
                      ETHERNET_HAS_INTERNET_BIT),
               (bool)(xEventGroupGetBits(s_connectivity_event_group) &
                      GSM_HAS_INTERNET_BIT),
               (bool)(xEventGroupGetBits(s_connectivity_event_group) &
                      EPPP_HAS_INTERNET_BIT));
    } else {
      xEventGroupClearBits(s_connectivity_event_group, DEVICE_HAS_INTERNET);
      ESP_LOGD(TAG, "DEVICE DOES NOT HAVE INTERNET!!!");
    }

    vTaskDelay(pdMS_TO_TICKS(3000));
  }

  // End new here
}
/// TODO: Implement auto logic
void connectivitychangeprovider(
    enum connectivity_network_provider_t
        provider) { // This is override func. Use this only if auto is not
                    // required
  current_provider = provider;
}

void init_connectivity(uint8_t *priority,
                       bool use_custom_internet_check_provider) {
  custom_internet_check_proder = use_custom_internet_check_provider;
  if (priority != NULL) {
    memcpy(user_priority, priority, NETWORK_PROVIDER_MAX * sizeof(uint8_t));
  } else {
    memcpy(user_priority, default_priority,
           NETWORK_PROVIDER_MAX * sizeof(uint8_t));
  }
  ESP_ERROR_CHECK(esp_netif_init());                // 4581
  ESP_ERROR_CHECK(esp_event_loop_create_default()); // 4087
  ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, ESP_EVENT_ANY_ID,
                                             &on_ip_event, NULL)); // 72
#ifdef CONFIG_ENABLE_MODEM
  ESP_ERROR_CHECK(esp_event_handler_register(NETIF_PPP_STATUS, ESP_EVENT_ANY_ID,
                                             &on_ppp_changed, NULL));
#endif

  if (s_connectivity_event_group == NULL) {
    s_connectivity_event_group = xEventGroupCreate(); // 40
  }

  // Init network interfaces
#ifdef CONFIG_ENABLE_MODEM
  // Modem
  init_modem();
#endif
#if CONFIG_ENABLE_WIFI
  // Wifi
  init_wifi_manager(); // 34240

#endif

#ifdef CONFIG_ENABLE_ETHERNET
  ethernet_init();
#endif

#ifdef CONFIG_ENABLE_EPPP_CLIENT
  eppp_client_init();
#endif

  xTaskCreate(&connectivity_task, "connectivity_task", 1024 * 3, NULL, 5,
              NULL); // 6672
}

// These 2 functions will be provided to the application to check for internet
// access and perform certian actions only if internet is available.

bool checkDeviceHasInternet() {
  return s_connectivity_event_group != NULL
             ? xEventGroupGetBits(s_connectivity_event_group) &
                   (DEVICE_HAS_INTERNET)
             : false;
}

void waitDeviceHasInternet() {
  while (s_connectivity_event_group == NULL) {
    vTaskDelay(pdMS_TO_TICKS(100));
  }
  xEventGroupWaitBits(s_connectivity_event_group, DEVICE_HAS_INTERNET, pdFALSE,
                      pdTRUE, portMAX_DELAY);
}

// This one might be required by tcp socket.
void waitDeviceHasIP() {
  xEventGroupWaitBits(s_connectivity_event_group,
                      WIFI_HAS_IP_BIT || ETHERNET_HAS_IP_BIT ||
                          GSM_HAS_IP_BIT || EPPP_HAS_IP_BIT,
                      pdFALSE, pdFALSE, portMAX_DELAY);
}

void getDeviceIP(ip_info_t *info) {
  xEventGroupGetBits(s_connectivity_event_group);
  if (xEventGroupGetBits(s_connectivity_event_group) & WIFI_HAS_IP_BIT) {
    ESP_LOGI(TAG, "Connected to Wifi Network");
    if (device_connection_info[NETWORK_PROVIDER_WIFI].netif_handle == NULL) {
      ESP_LOGE(TAG, "Netif handle is null");
      info[NETWORK_PROVIDER_WIFI].connected = false;
    } else {
      wifi_ap_record_t ap_info;
      esp_wifi_sta_get_ap_info(&ap_info);
      memcpy(info[NETWORK_PROVIDER_WIFI].ssid, ap_info.ssid, 32);
      info[NETWORK_PROVIDER_WIFI].connected = true;
      const char *netif_desc = esp_netif_get_desc(
          device_connection_info[NETWORK_PROVIDER_WIFI].netif_handle);
      memcpy(info[NETWORK_PROVIDER_WIFI].description, netif_desc, 32);
      // Convert IP information to string
      char ip_str[16] = {0};
      char netmask_str[16] = {0};
      char gw_str[16] = {0};
      snprintf(
          ip_str, 16, IPSTR,
          IP2STR(&device_connection_info[NETWORK_PROVIDER_WIFI].ip_info.ip));
      snprintf(
          netmask_str, 16, IPSTR,
          IP2STR(
              &device_connection_info[NETWORK_PROVIDER_WIFI].ip_info.netmask));
      snprintf(
          gw_str, 16, IPSTR,
          IP2STR(&device_connection_info[NETWORK_PROVIDER_WIFI].ip_info.gw));
      memcpy(info[NETWORK_PROVIDER_WIFI].ip_address, ip_str, 16);
      memcpy(info[NETWORK_PROVIDER_WIFI].netmask, netmask_str, 16);
      memcpy(info[NETWORK_PROVIDER_WIFI].gateway, gw_str, 16);
    }
  }
  if (xEventGroupGetBits(s_connectivity_event_group) & ETHERNET_HAS_IP_BIT) {
    ESP_LOGI(TAG, "Connected to Ethernet network");
    if (device_connection_info[NETWORK_PROVIDER_ETHERNET].netif_handle ==
        NULL) {
      ESP_LOGE(TAG, "Netif handle is null");
      info[NETWORK_PROVIDER_ETHERNET].connected = false;
    } else {
      info[NETWORK_PROVIDER_ETHERNET].connected = true;
      const char *netif_desc = esp_netif_get_desc(
          device_connection_info[NETWORK_PROVIDER_ETHERNET].netif_handle);
      memcpy(info[NETWORK_PROVIDER_ETHERNET].description, netif_desc, 32);
      // Convert IP information to string
      char ip_str[16] = {0};
      char netmask_str[16] = {0};
      char gw_str[16] = {0};
      snprintf(
          ip_str, 16, IPSTR,
          IP2STR(
              &device_connection_info[NETWORK_PROVIDER_ETHERNET].ip_info.ip));
      snprintf(netmask_str, 16, IPSTR,
               IP2STR(&device_connection_info[NETWORK_PROVIDER_ETHERNET]
                           .ip_info.netmask));
      snprintf(
          gw_str, 16, IPSTR,
          IP2STR(
              &device_connection_info[NETWORK_PROVIDER_ETHERNET].ip_info.gw));
      memcpy(info[NETWORK_PROVIDER_ETHERNET].ip_address, ip_str, 16);
      memcpy(info[NETWORK_PROVIDER_ETHERNET].netmask, netmask_str, 16);
      memcpy(info[NETWORK_PROVIDER_ETHERNET].gateway, gw_str, 16);
    }
  }
  if (xEventGroupGetBits(s_connectivity_event_group) & GSM_HAS_IP_BIT) {
    ESP_LOGI(TAG, "Connected to GSM network");
    if (device_connection_info[NETWORK_PROVIDER_MODEM].netif_handle == NULL) {
      ESP_LOGE(TAG, "Netif handle is null");
      info[NETWORK_PROVIDER_MODEM].connected = false;
    } else {
      info[NETWORK_PROVIDER_MODEM].connected = true;
      const char *netif_desc = esp_netif_get_desc(
          device_connection_info[NETWORK_PROVIDER_MODEM].netif_handle);
      memcpy(info[NETWORK_PROVIDER_MODEM].description, netif_desc, 32);
      // Convert IP information to string
      char ip_str[16] = {0};
      char netmask_str[16] = {0};
      char gw_str[16] = {0};
      snprintf(
          ip_str, 16, IPSTR,
          IP2STR(&device_connection_info[NETWORK_PROVIDER_MODEM].ip_info.ip));
      snprintf(
          netmask_str, 16, IPSTR,
          IP2STR(
              &device_connection_info[NETWORK_PROVIDER_MODEM].ip_info.netmask));
      snprintf(
          gw_str, 16, IPSTR,
          IP2STR(&device_connection_info[NETWORK_PROVIDER_MODEM].ip_info.gw));
      memcpy(info[NETWORK_PROVIDER_MODEM].ip_address, ip_str, 16);
      memcpy(info[NETWORK_PROVIDER_MODEM].netmask, netmask_str, 16);
      memcpy(info[NETWORK_PROVIDER_MODEM].gateway, gw_str, 16);
    }
  }
  if (xEventGroupGetBits(s_connectivity_event_group) & EPPP_HAS_IP_BIT) {
    ESP_LOGI(TAG, "Connected to EPPP network");
    if (device_connection_info[NETWORK_PROVIDER_EPPP_LINK].netif_handle ==
        NULL) {
      ESP_LOGE(TAG, "Netif handle is null");
      info[NETWORK_PROVIDER_EPPP_LINK].connected = false;
    } else {
      info[NETWORK_PROVIDER_EPPP_LINK].connected = true;
      const char *netif_desc = esp_netif_get_desc(
          device_connection_info[NETWORK_PROVIDER_EPPP_LINK].netif_handle);
      memcpy(info[NETWORK_PROVIDER_EPPP_LINK].description, netif_desc, 32);
      // Convert IP information to string
      char ip_str[16] = {0};
      char netmask_str[16] = {0};
      char gw_str[16] = {0};
      snprintf(
          ip_str, 16, IPSTR,
          IP2STR(
              &device_connection_info[NETWORK_PROVIDER_EPPP_LINK].ip_info.ip));
      snprintf(netmask_str, 16, IPSTR,
               IP2STR(&device_connection_info[NETWORK_PROVIDER_EPPP_LINK]
                           .ip_info.netmask));
      snprintf(
          gw_str, 16, IPSTR,
          IP2STR(
              &device_connection_info[NETWORK_PROVIDER_EPPP_LINK].ip_info.gw));
      memcpy(info[NETWORK_PROVIDER_EPPP_LINK].ip_address, ip_str, 16);
      memcpy(info[NETWORK_PROVIDER_EPPP_LINK].netmask, netmask_str, 16);
      memcpy(info[NETWORK_PROVIDER_EPPP_LINK].gateway, gw_str, 16);
    }
  }
}