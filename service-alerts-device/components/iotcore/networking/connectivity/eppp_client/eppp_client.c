#include "eppp_client.h"
#include "eppp_link.h"
#include "esp_log.h"

static const char *TAG = "eppp_client";
esp_netif_t *eppp_netif = NULL;
#define GLOBAL_DNS 0x08080808
void eppp_client_init(void) {
  eppp_config_t config = get_eppp_config(EPPP_CLIENT);
  eppp_netif = eppp_connect(&config);
  if (eppp_netif == NULL) {
    ESP_LOGE(TAG, "Failed to connect");
    return;
  }
}

void eppp_client_start() {
  esp_netif_dns_info_t dns;
  dns.ip.u_addr.ip4.addr = esp_netif_htonl(GLOBAL_DNS);
  dns.ip.type = ESP_IPADDR_TYPE_V4;
  ESP_ERROR_CHECK(esp_netif_set_dns_info(eppp_netif, ESP_NETIF_DNS_MAIN, &dns));
}

void eppp_client_stop() {
  eppp_close(eppp_netif);
  eppp_netif = NULL;
}