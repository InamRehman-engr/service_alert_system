#ifndef _th_i2c_h_
#define _th_i2c_h_

#include "esp_err.h"
#include <stdint.h>

#define I2C_MASTER_NUM I2C_NUM_0 /*!< I2C port number for master dev */

typedef struct {
  void (*fun_ptr)(void);
  uint32_t measurement_time_sec;
} temp_humidity_sensor_t;

esp_err_t i2c_master_init(const uint32_t i2c_clk_freq);

#endif
