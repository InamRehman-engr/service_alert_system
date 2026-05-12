
#include "th_i2c.h"
#include "driver/i2c.h"
#include "esp_log.h"
#include "sdkconfig.h"
#include <stdio.h>

#define I2C_MASTER_SCL_IO                                                      \
  CONFIG_TEMP_HUM_I2C_SCL_PIN /*!< gpio number for I2C master clock */
#define I2C_MASTER_SDA_IO                                                      \
  CONFIG_TEMP_HUM_I2C_SDA_PIN       /*!< gpio number for I2C master data  */
#define I2C_MASTER_TX_BUF_DISABLE 0 /*!< I2C master doesn't need buffer */
#define I2C_MASTER_RX_BUF_DISABLE 0 /*!< I2C master doesn't need buffer */

esp_err_t i2c_master_init(const uint32_t i2c_clk_freq) {
  int i2c_master_port = I2C_MASTER_NUM;
  i2c_config_t conf;
  conf.mode = I2C_MODE_MASTER;
  conf.sda_io_num = I2C_MASTER_SDA_IO;
  conf.sda_pullup_en = GPIO_PULLUP_ENABLE;
  conf.scl_io_num = I2C_MASTER_SCL_IO;
  conf.scl_pullup_en = GPIO_PULLUP_ENABLE;
  conf.master.clk_speed = i2c_clk_freq;
  conf.clk_flags = 0;
  i2c_param_config(i2c_master_port, &conf);
  return i2c_driver_install(i2c_master_port, conf.mode,
                            I2C_MASTER_RX_BUF_DISABLE,
                            I2C_MASTER_TX_BUF_DISABLE, 0);
}
