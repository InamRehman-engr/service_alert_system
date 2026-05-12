#ifndef __OPT1001__
#define __OPT1001__

#include "driver/gpio.h"
#include "driver/i2c.h"
#include "esp_log.h"
#include "esp_system.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "sdkconfig.h"
#include <inttypes.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>

#define SMBUS_DEFAULT_TIMEOUT                                                  \
  (1000 / portTICK_PERIOD_MS) ///< Default transaction timeout in ticks

/**
 * @brief 7-bit or 10-bit I2C slave address.
 */
typedef uint16_t i2c_address_t;

/**
 * @brief Structure containing information related to the SMBus protocol.
 */
typedef struct {
  bool init;           ///< True if struct has been initialised, otherwise false
  i2c_port_t i2c_port; ///< ESP-IDF I2C port number
  i2c_address_t address; ///< I2C address of slave device
  portBASE_TYPE timeout; ///< Number of ticks until I2C operation timeout
} smbus_info_t;

/**
 * @brief Construct a new SMBus info instance.
 *        New instance should be initialised before calling other functions.
 * @return Pointer to new device info instance, or NULL if it cannot be created.
 */
smbus_info_t *smbus_malloc(void);

/**
 * @brief Delete an existing SMBus info instance.
 * @param[in,out] smbus_info Pointer to SMBus info instance that will be freed
 * and set to NULL.
 */
void smbus_free(smbus_info_t **smbus_info);

/**
 * @brief Initialise a SMBus info instance with the specified I2C information.
 *        The I2C timeout defaults to approximately 1 second.
 * @param[in] smbus_info Pointer to SMBus info instance.
 * @param[in] i2c_port I2C port to associate with this SMBus instance.
 * @param[in] address Address of I2C slave device.
 */
esp_err_t smbus_init(smbus_info_t *smbus_info, i2c_port_t i2c_port,
                     i2c_address_t address);

/**
 * @brief Set the I2C timeout.
 *        I2C transactions that do not complete within this period are
 * considered an error.
 * @param[in] smbus_info Pointer to initialised SMBus info instance.
 * @param[in] timeout Number of ticks to wait until the transaction is
 * considered in error.
 * @return ESP_OK if successful, ESP_FAIL or ESP_ERR_* if an error occurred.
 */
esp_err_t smbus_set_timeout(smbus_info_t *smbus_info, portBASE_TYPE timeout);

/**
 * @brief Send a single bit to a slave device in the place of the read/write
 * bit. May be used to simply turn a device function on or off, or enable or
 * disable a low-power standby mode. There is no data sent or received.
 * @param[in] smbus_info Pointer to initialised SMBus info instance.
 * @param[in] bit Data bit to send.
 * @return ESP_OK if successful, ESP_FAIL or ESP_ERR_* if an error occurred.
 */
esp_err_t smbus_quick(const smbus_info_t *smbus_info, bool bit);

/**
 * @brief Write a single word (two bytes) to a slave device with a command code.
 *        The most significant byte is transmitted first.
 * @param[in] smbus_info Pointer to initialised SMBus info instance.
 * @param[in] command Device-specific command byte.
 * @param[in] data Data word to send to slave.
 * @return ESP_OK if successful, ESP_FAIL or ESP_ERR_* if an error occurred.
 */
esp_err_t smbus_write_word(const smbus_info_t *smbus_info, uint8_t command,
                           uint16_t data);

/**
 * @brief Read a single word (two bytes) from a slave device with a command
 * code. The first byte received is the least significant byte.
 * @param[in] smbus_info Pointer to initialised SMBus info instance.
 * @param[in] command Device-specific command byte.
 * @param[out] data Data byte received from slave.
 * @return ESP_OK if successful, ESP_FAIL or ESP_ERR_* if an error occurred.
 */
esp_err_t smbus_read_word(const smbus_info_t *smbus_info, uint8_t command,
                          uint16_t *data);

typedef uint16_t device_type_t;   ///< The type of Device
typedef uint16_t data_register_t; ///< The type of the IC's Data value

/**
 * @brief Structure containing information related to the SMBus protocol.
 */
typedef struct {
  bool init; ///< True if struct has been initialised, otherwise false
  smbus_info_t *smbus_info;  ///< Pointer to associated SMBus info
  device_type_t device_type; ///< Detected type of device
} opt3001_info_t;

/**
 * @brief Construct a new OPT3001 info instance.
 *        New instance should be initialised before calling other functions.
 * @return Pointer to new device info instance, or NULL if it cannot be created.
 */
opt3001_info_t *opt3001_malloc(void);

/**
 * @brief Delete an existing OPT3001 info instance.
 * @param[in,out] opt3001_info Pointer to OPT3001 info instance that will be
 * freed and set to NULL.
 */
void opt3001_free(opt3001_info_t **opt3001_info);

/**
 * @brief Initialise a OPT3001 info instance with the specified SMBus
 * information.
 * @param[in] opt3001_info Pointer to OPT3001 info instance.
 * @param[in] smbus_info Pointer to SMBus info instance.
 */
esp_err_t opt3001_init(opt3001_info_t *opt3001_info, smbus_info_t *smbus_info);

/**
 * @brief Retrieve the Device Type ID and Revision number from the device.
 * @param[in] opt3001_info Pointer to initialised opt3001 info instance.
 * @param[out] device The retrieved Device Type ID.
 * @param[out] revision The retrieved Device Revision number.
 * @return ESP_OK if successful, ESP_FAIL or ESP_ERR_* if an error occurred.
 */
esp_err_t opt3001_device_id(const opt3001_info_t *opt3001_info,
                            device_type_t *device);

/**
 * @brief Retrieve a visible and infrared light measurement from the device.
 * @param[in] opt3001_info Pointer to initialised opt3001 info instance.
 * @param[out] lower_byte The resultant lower byte of the measurement.
 * @param[out] upper_byte The resultant upper byte of the measurement.
 * @return ESP_OK if successful, ESP_FAIL or ESP_ERR_* if an error occurred.
 */
esp_err_t opt3001_read(opt3001_info_t *opt3001_info,
                       data_register_t *lower_byte,
                       data_register_t *upper_byte);

/**
 * @brief Compute the Lux approximation from a visible and infrared light
 * measurement. The calculation is performed according to the procedure given in
 * the datasheet.
 * @param[in] opt3001_info Pointer to initialised opt3001 info instance.
 * @param[out] lower_byte The resultant lower byte of the measurement.
 * @param[out] upper_byte The resultant upper byte of the measurement.
 * @return The resulting approximation of the light measurement in Lux.
 */
// uint32_t opt3001_compute_lux(const opt3001_info_t *opt3001_info,
// data_register_t *lower_byte, data_register_t *upper_byte);

void OPT3001_i2c_master_init(void);
void initialize_opt3001(void);
void get_sensor_values(void);
#endif // __OPT1001__
