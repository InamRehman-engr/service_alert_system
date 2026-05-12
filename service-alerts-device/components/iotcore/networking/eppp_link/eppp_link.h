#pragma once

#include "esp_err.h"
#include "esp_netif.h"
#include "esp_types.h"
#include <stdbool.h>

typedef enum eppp_type {
  EPPP_SERVER,
  EPPP_CLIENT,
} eppp_type_t;

typedef enum eppp_transport {
  EPPP_TRANSPORT_UART,
  EPPP_TRANSPORT_SPI,
  EPPP_TRANSPORT_SDIO,
} eppp_transport_t;

typedef struct eppp_spi_config_t {
  int host;
  int mosi;
  int miso;
  int sclk;
  int cs;
  int intr;
  int freq;
  int input_delay_ns;
  int cs_ena_pretrans;
  int cs_ena_posttrans;
} eppp_spi_config_t;

typedef struct eppp_eppp_uart_config_t {
  int port;
  int baud;
  int tx_io;
  int rx_io;
  int queue_size;
  int rx_buffer_size;
} eppp_uart_config_t;

typedef struct eppp_eppp_sdio_config_t {
  int width;
  int clk;
  int cmd;
  int d0;
  int d1;
  int d2;
  int d3;
} eppp_sdio_config_t;

typedef struct eppp_config_t {
  eppp_transport_t transport;

  union {
    eppp_spi_config_t spi;
    eppp_uart_config_t uart;
    eppp_sdio_config_t sdio;
  } transport_config;

  struct eppp_config_task_s {
    bool run_task;
    int stack_size;
    int priority;
  } task;

  struct eppp_config_pppos_s {
    esp_ip4_addr_t our_ip4_addr;
    esp_ip4_addr_t their_ip4_addr;
    int netif_prio;
    const char *netif_description;
  } ppp;

} eppp_config_t;

esp_netif_t *eppp_connect(eppp_config_t *config);

esp_netif_t *eppp_listen(eppp_config_t *config);

void eppp_close(esp_netif_t *netif);

esp_netif_t *eppp_init(eppp_type_t role, eppp_config_t *config);

void eppp_deinit(esp_netif_t *netif);

esp_netif_t *eppp_open(eppp_type_t role, eppp_config_t *config,
                       int connect_timeout_ms);

esp_err_t eppp_netif_stop(esp_netif_t *netif, int stop_timeout_ms);

esp_err_t eppp_netif_start(esp_netif_t *netif);

esp_err_t eppp_perform(esp_netif_t *netif);

eppp_config_t get_eppp_config(eppp_type_t role);