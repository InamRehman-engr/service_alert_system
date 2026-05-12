#include "ethernet.h"
#include <esp_netif.h>

static const char *TAG = "ETHERNET";

#if CONFIG_ENABLE_INTERNAL_ETHERNET
esp_eth_handle_t eth_handle = NULL;
esp_netif_t *eth_netif = {NULL};
#endif

#if CONFIG_ENABLE_SPI_ETHERNET
esp_eth_handle_t eth_handle_spi;
esp_netif_t *eth_netif_spi = {NULL};
#endif

uint8_t mac_addr[6] = {0};

// Function to use next MAC from internal ethernet MAC of ESP32 as the MAC for
// SPI Ethernet chip. Call this in ethernet_init() only if chip does not have a
// default mac address.
void bind_MAC() {
  esp_read_mac((uint8_t *)mac_addr, ESP_MAC_ETH);
  ESP_LOGD(TAG, "Internal MAC " MACSTR, MAC2STR(mac_addr));
  mac_addr[5] = mac_addr[5] + 1;
  ESP_LOGD(TAG, "Binded MAC " MACSTR, MAC2STR(mac_addr));
}

static void eth_event_handler(void *arg, esp_event_base_t event_base,
                              int32_t event_id, void *event_data) {

  esp_eth_handle_t eth_handle = *(esp_eth_handle_t *)event_data;

  switch (event_id) {
  case ETHERNET_EVENT_CONNECTED:
    esp_eth_ioctl(eth_handle, ETH_CMD_G_MAC_ADDR, mac_addr);
    ESP_LOGI(TAG, "Ethernet Link Up");
    ESP_LOGD(TAG, "Ethernet HW Addr %02x:%02x:%02x:%02x:%02x:%02x", mac_addr[0],
             mac_addr[1], mac_addr[2], mac_addr[3], mac_addr[4], mac_addr[5]);
    break;
  case ETHERNET_EVENT_DISCONNECTED:
    ESP_LOGI(TAG, "Ethernet Link Down");
    break;
  case ETHERNET_EVENT_START:
    ESP_LOGD(TAG, "Ethernet Started");
    break;
  case ETHERNET_EVENT_STOP:
    ESP_LOGD(TAG, "Ethernet Stopped");
    break;
  default:
    break;
  }
}

/**
 * Calling this requires that you have SPI initialization done before this.
 */
static void ethernet_init_task(void *arg) {
#if CONFIG_ENABLE_INTERNAL_ETHERNET

  esp_netif_config_t cfg = ESP_NETIF_DEFAULT_ETH();
  eth_netif = esp_netif_new(&cfg);
  eth_mac_config_t mac_config = ETH_MAC_DEFAULT_CONFIG();
  eth_phy_config_t phy_config = ETH_PHY_DEFAULT_CONFIG();
  phy_config.phy_addr = CONFIG_ETH_PHY_ADDR;
  phy_config.reset_gpio_num = CONFIG_ETH_PHY_RST_GPIO;
  eth_esp32_emac_config_t esp32_emac_config = ETH_ESP32_EMAC_DEFAULT_CONFIG();
  esp32_emac_config.smi_mdc_gpio_num = CONFIG_ETH_MDC_GPIO;
  esp32_emac_config.smi_mdio_gpio_num = CONFIG_ETH_MDIO_GPIO;
  esp_eth_mac_t *mac = esp_eth_mac_new_esp32(&esp32_emac_config, &mac_config);

  esp_eth_phy_t *phy = esp_eth_phy_new_lan87xx(&phy_config);

  esp_eth_config_t config = ETH_DEFAULT_CONFIG(mac, phy);
  ESP_ERROR_CHECK(esp_eth_driver_install(&config, &eth_handle));
  ESP_ERROR_CHECK(
      esp_netif_attach(eth_netif, esp_eth_new_netif_glue(eth_handle)));
#endif

#if CONFIG_ENABLE_SPI_ETHERNET

  bind_MAC(); // Used because W5500 does not have a default MAC

  esp_netif_inherent_config_t esp_netif_config =
      ESP_NETIF_INHERENT_DEFAULT_ETH();
  esp_netif_config_t cfg_spi = {.base = &esp_netif_config,
                                .stack = ESP_NETIF_NETSTACK_DEFAULT_ETH};
  char if_key_str[10];
  char if_desc_str[10];
  char num_str[3];
  itoa(0, num_str, 10);
  strcat(strcpy(if_key_str, "ETH_SPI_"), num_str);
  strcat(strcpy(if_desc_str, "eth"), num_str);
  esp_netif_config.if_key = if_key_str;
  esp_netif_config.if_desc = if_desc_str;
  esp_netif_config.route_prio = 30;
  eth_netif_spi = esp_netif_new(&cfg_spi);

  eth_mac_config_t mac_config_spi = ETH_MAC_DEFAULT_CONFIG();
  eth_phy_config_t phy_config_spi = ETH_PHY_DEFAULT_CONFIG();

  gpio_install_isr_service(0);

  spi_eth_module_config_t spi_eth_module_config;
  spi_eth_module_config.spi_cs_gpio = CONFIG_ETH_SPI_CS_PIN;
  spi_eth_module_config.int_gpio = CONFIG_ETH_SPI_INT_PIN;
  spi_eth_module_config.phy_reset_gpio = CONFIG_ETH_SPI_RST_PIN;
  spi_eth_module_config.phy_addr = CONFIG_ETH_PHY_ADDR;

  esp_eth_mac_t *mac_spi;
  esp_eth_phy_t *phy_spi;
  spi_device_interface_config_t spi_devcfg = {
      .mode = 0,
      .clock_speed_hz = CONFIG_ETH_SPI_CLK_SPEED * 1000 * 1000,
      .queue_size = 20};

  spi_devcfg.spics_io_num = spi_eth_module_config.spi_cs_gpio;

  phy_config_spi.phy_addr = spi_eth_module_config.phy_addr;
  phy_config_spi.reset_gpio_num = spi_eth_module_config.phy_reset_gpio;

  eth_w5500_config_t w5500_config =
      ETH_W5500_DEFAULT_CONFIG(CONFIG_ETH_SPI_HOST, &spi_devcfg);
  w5500_config.int_gpio_num = spi_eth_module_config.int_gpio;
  mac_spi = esp_eth_mac_new_w5500(&w5500_config, &mac_config_spi);
  phy_spi = esp_eth_phy_new_w5500(&phy_config_spi);

  esp_eth_config_t eth_config_spi = ETH_DEFAULT_CONFIG(mac_spi, phy_spi);

  esp_err_t err;
  while (1) {
    err = esp_eth_driver_install(&eth_config_spi, &eth_handle_spi);
    if (err == ESP_OK) {
      ESP_LOGI(TAG, "Ethernet driver initialized successfully!");
      break;
    } else {
      ESP_LOGE(TAG, "Ethernet driver initialization failed, retrying...");
      vTaskDelay(pdMS_TO_TICKS(3000));
    }
  }

  if (esp_eth_ioctl(eth_handle_spi, ETH_CMD_S_MAC_ADDR, mac_addr)) {
    ESP_LOGE(TAG, "Ethernet IO control failed to initialize!");
    vTaskDelete(NULL);
  }

  if (esp_netif_attach(eth_netif_spi, esp_eth_new_netif_glue(eth_handle_spi))) {
    ESP_LOGE(TAG, "Ethernet netif failed to attach!");
    vTaskDelete(NULL);
  }
  ESP_LOGI(TAG, "Ethernet SPI initialization completed.");
#endif
  vTaskDelete(NULL);
}
void ethernet_init() {
  xTaskCreate(&ethernet_init_task, "eth_init_task", 3 * 1024, NULL, 10, NULL);
  ESP_ERROR_CHECK(esp_event_handler_register(ETH_EVENT, ESP_EVENT_ANY_ID,
                                             &eth_event_handler, NULL));
}

esp_err_t ethernet_start() {
#if CONFIG_ENABLE_INTERNAL_ETHERNET
  return esp_eth_start(eth_handle);
#endif

#if CONFIG_ENABLE_SPI_ETHERNET
  if (eth_handle_spi != NULL) {
    return esp_eth_start(eth_handle_spi);
  }
#endif
  return ESP_ERR_NOT_FOUND;
}

void ethernet_stop() {
#if CONFIG_ENABLE_INTERNAL_ETHERNET
  esp_eth_stop(eth_handle);
#endif

#if CONFIG_ENABLE_SPI_ETHERNET
  if (eth_handle_spi != NULL) {
    esp_eth_stop(eth_handle_spi);
  }
#endif
  // ESP_ERROR_CHECK(esp_event_handler_unregister(ETH_EVENT, ESP_EVENT_ANY_ID,
  // &eth_event_handler)); esp_netif_destroy(eth_netif_spi);
  // ESP_ERROR_CHECK(esp_eth_driver_uninstall(eth_handle_spi));
  // ESP_ERROR_CHECK(spi_bus_free(CONFIG_ETH_SPI_HOST));
  // gpio_uninstall_isr_service();
}