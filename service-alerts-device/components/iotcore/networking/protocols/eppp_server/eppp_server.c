#include "eppp_server.h"
#include "connectivity.h"
#include "eppp_link.h"
#include "esp_err.h"
#include "esp_log.h"

static const char *TAG = "eppp_server";
static esp_netif_t *eppp_server_netif = NULL;
void eppp_server_init() {
  eppp_config_t config = get_eppp_config(EPPP_SERVER);
  eppp_server_netif = eppp_listen(&config);
  if (eppp_server_netif == NULL) {
    ESP_LOGE(TAG, "Failed to setup connection");
    return;
  }
  ESP_ERROR_CHECK(esp_netif_napt_enable(eppp_server_netif));
}

void eppp_server_stop() {
  eppp_close(eppp_server_netif);
  eppp_server_netif = NULL;
}