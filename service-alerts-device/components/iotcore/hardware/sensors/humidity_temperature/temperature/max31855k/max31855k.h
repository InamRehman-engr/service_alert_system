#ifndef _max31855k_h
#define _max31855k_h

#include "esp_err.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include <spi_master_dev.h>

typedef struct {

} max31855_data;

typedef struct {
  spi_master_functions max31855k;
  float thermocoupleTemperature;
  float junctionTemperature;
  float temperature;
  bool faultBit;
  bool shortCircuitHighBit;
  bool shortCircuitLowBit;
  bool openCircuitBit;
} max31855k_handle_t;

esp_err_t max31855k_getData(max31855k_handle_t *handle);

#endif //_max31855k_h