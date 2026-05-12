#ifndef _I2C_DRIVER_H
#define _I2C_DRIVER_H

#include "driver/i2c.h"
#include "i2c_driver.h"
#include <stdint.h>

void vI2CInit();
esp_err_t sI2cMasterReadSlave(uint8_t *data_rd, size_t size,
                              uint8_t slave_address);
esp_err_t sI2cMasterWriteSlave(uint8_t *data_wr, size_t size,
                               uint8_t slave_address);

#endif
