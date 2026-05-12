#ifndef _ethernet_h_
#define _ethernet_h_

#include "driver/gpio.h"
#include "driver/spi_master.h"
#include "esp_eth.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_mac.h"
#include "esp_netif.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "hal/spi_types.h"
#include "sdkconfig.h"
#include "time.h"
#include <stdio.h>
#include <string.h>

typedef struct {
  uint8_t spi_cs_gpio;
  uint8_t int_gpio;
  int8_t phy_reset_gpio;
  uint8_t phy_addr;
} spi_eth_module_config_t;

void ethernet_init(void);
esp_err_t ethernet_start(void);
void ethernet_stop(void);

#endif