#include "ppsi262.h"
#include "driver/i2c.h"
#include "esp_err.h"
#include "esp_log.h"
#include "esp_types.h"

// #######################----I2C
// Read/Write---##################################################
static esp_err_t readResponseBytes(const i2c_port_t i2c_num, const unint8_t reg,
                                   uint8_t *output, const size_t nbytes) {
  i2c_cmd_handle_t cmd = i2c_cmd_link_create();
  i2c_master_start(cmd);

// write the 7-bit address of the sensor to the queue, using the last bit
// to indicate we are performing a read.
#ifndef I2C_SLAVEADDR_SENPIN_HIGH
  i2c_master_write_byte(cmd, I2C_SLAVEADDR_SENPIN_HIGH << 1 | I2C_MASTER_READ,
                        ACK_CHECK_EN);
#endif
#ifndef I2C_SLAVEADDR_SENPIN_LOw
  i2c_master_write_byte(cmd, I2C_SLAVEADDR_SENPIN_LOW << 1 | I2C_MASTER_READ,
                        ACK_CHECK_EN);
#endif

  i2c_master_write_byte(cmd, reg, ACK_CHECK_EN);

  // Repeeated start condition for this Library as mentioned in the data sheet
  i2c_master_start(cmd);

// See the header file for why the write function is different
#ifndef I2C_SLAVEADDR_SENPIN_HIGH
  i2c_master_write_byte(cmd, I2C_SLAVEADDR_SENPIN_HIGH << 1 | I2C_MASTER_READ,
                        ACK_CHECK_EN);
#endif
#ifndef I2C_SLAVEADDR_SENPIN_LOw
  i2c_master_write_byte(cmd, I2C_SLAVEADDR_SENPIN_LOW << 1 | I2C_MASTER_READ,
                        ACK_CHECK_EN);
#endif

  // read nbytes number of bytes from the response into the buffer. make
  // sure we send a NACK with the final byte rather than an ACK.
  for (size_t i = 0; i < nbytes; i++) {
    i2c_master_read_byte(cmd, &output[i], i == nbytes - 1 ? NACK_VAL : ACK_VAL);
  }

  // send all queued commands, blocking until all commands have been sent.
  // note that this is *not* thread-safe.
  i2c_master_stop(cmd);
  esp_err_t ret =
      i2c_master_cmd_begin(i2c_num, cmd, I2C_TIMEOUT_MS / portTICK_PERIOD_MS);
  i2c_cmd_link_delete(cmd);

  return ret;
}

static esp_err_t writeCommandBytes(const i2c_port_t i2c_num, const uint8_t reg,
                                   const uint8_t *i2c_command,
                                   const size_t nbytes) {
  i2c_cmd_handle_t cmd = i2c_cmd_link_create();
  i2c_master_start(cmd);

// write the 7-bit address of the sensor to the bus, using the last bit to
// indicate we are performing a write.
#ifndef I2C_SLAVEADDR_SENPIN_HIGH
  i2c_master_write_byte(cmd, I2C_SLAVEADDR_SENPIN_HIGH << 1 | I2C_MASTER_READ,
                        ACK_CHECK_EN);
#endif
#ifndef I2C_SLAVEADDR_SENPIN_LOw
  i2c_master_write_byte(cmd, I2C_SLAVEADDR_SENPIN_LOW << 1 | I2C_MASTER_READ,
                        ACK_CHECK_EN);
#endif

  i2c_master_write_byte(cmd, reg, ACK_CHECK_EN);

  for (size_t i = 0; i < nbytes; i++) {
    i2c_master_write_byte(cmd, i2c_command[i], 1);
  }
  // send all queued commands, blocking until all commands have been sent.
  // note that this is *not* thread-safe.
  i2c_master_stop(cmd);
  esp_err_t ret =
      i2c_master_cmd_begin(i2c_num, cmd, I2C_TIMEOUT_MS / portTICK_PERIOD_MS);
  i2c_cmd_link_delete(cmd);

  return ret;
}

// Function to decode the ADC value by 2s complement
static uint32_t twos_complement_adc_value(uint32_t reada_complement_dcvalue) {
  uint32_t adcvalue = ~reada_complement_dcvalue + 1;
  return adcvalue;
}
// #######################----I2C
// Read/Write---##################################################