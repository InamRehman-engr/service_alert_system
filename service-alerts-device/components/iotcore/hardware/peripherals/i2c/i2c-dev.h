#ifndef __I2C_H__
#define __I2C_H__
// Master Mode
/*
Usage of this driver is as follows
- The user needs to call the initializer function.
- The underlying device needs to implement the funcions necessary to use the
object of the i2c structure returned by this file
- I2C transactions can fail. any and all retry mechanisms and data integrity
checks need to be implemented by the device side Implementation will be similar
bbut not the same to the linux i2c-dev driver

ESP's internal I2C driver include locking but it will throw error if the bus is
being used. This abstraction layer serves the following purposes
1. It provides a way to lock the bus while a transaction is being performed by a
device.
2. Serves as a wrapper to the original driver so if the idf upgrade changes in
any way this is the only file that will be changed without affecting the rest of
the code
*/

#include "esp_err.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "freertos/task.h"
#include "hal/i2c_types.h"
#include "sdkconfig.h"
#include "string.h"
#include <driver/i2c.h>
#include <stdio.h>
#define ESP_ERR_MUTEX_FAILED (ESP_ERR_WIFI_BASE | 0x1000)
#define I2C_MASTER_TX_BUF_DISABLE 0 /*!< I2C master doesn't need buffer */
#define I2C_MASTER_RX_BUF_DISABLE 0 /*!< I2C master doesn't need buffer */
#define I2C_MASTER_TIMEOUT_MS 1000
#define I2C_MUTEX_TIMEOUT_MS 3000
#define WRITE_BIT 0       /*!< I2C master write */
#define READ_BIT 1        /*!< I2C master read */
#define ACK_CHECK_EN 0x1  /*!< I2C master will check ack from slave*/
#define ACK_CHECK_DIS 0x0 /*!< I2C master will not check ack from slave */
#define ACK_VALUE 0x0
#define NACK_VALUE 0x1 /*!< I2C nack value */
#define ACK_CHECK true
#define NO_ACK_CHECK false

typedef struct {
  SemaphoreHandle_t i2c_mutex;
  int port;
} i2c_device_t;
typedef struct {
  i2c_device_t *device;
  esp_err_t (*i2c_send)(uint8_t addr, uint8_t *data, size_t data_len,
                        i2c_device_t *device);
  esp_err_t (*i2c_receive)(uint8_t addr, uint8_t *data, size_t data_len,
                           i2c_device_t *device);
  esp_err_t (*i2c_send_receive)(uint8_t addr, uint8_t *data_send,
                                size_t data_send_len, uint8_t *data_receive,
                                size_t data_receive_len, i2c_device_t *device);
  esp_err_t (*device_available)(uint8_t addr, i2c_device_t *device);
#ifdef CONFIG_SMBUS_SUPPORT
  esp_err_t (*smbus_send_byte)(uint8_t address, uint8_t data,
                               i2c_device_t *device);
  esp_err_t (*smbus_receive_byte)(uint8_t address, uint8_t *data,
                                  i2c_device_t *device);
  esp_err_t (*smbus_send_bytes)(uint8_t address, uint8_t command, uint8_t *data,
                                size_t len, i2c_device_t *device);
  esp_err_t (*smbus_receive_bytes)(uint8_t address, uint8_t command,
                                   uint8_t *data, size_t len,
                                   i2c_device_t *device);
  esp_err_t (*smbus_send_block)(uint8_t address, uint8_t command, uint8_t *data,
                                size_t len, i2c_device_t *device);
  esp_err_t (*smbus_receive_block)(uint8_t address, uint8_t command,
                                   uint8_t *data, uint8_t len,
                                   i2c_device_t *device);
  esp_err_t (*smbus_send_word)(uint8_t address, uint8_t command, uint8_t *data,
                               i2c_device_t *device);
  esp_err_t (*smbus_receive_word)(uint8_t address, uint8_t command,
                                  uint8_t *data, i2c_device_t *device);
#endif
} i2c_functions;
void i2c_init(i2c_mode_t mode, int sda_io, int scl_io, bool sda_pullup_en,
              bool scl_pullup_en, uint32_t clk_speed, i2c_functions *functions);
#endif