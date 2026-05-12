#ifndef TSL2561_H
#define TSL2561_H

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
 * @brief Send a single byte to a slave device.
 * @param[in] smbus_info Pointer to initialised SMBus info instance.
 * @param[in] data Data byte to send to slave.
 * @return ESP_OK if successful, ESP_FAIL or ESP_ERR_* if an error occurred.
 */
esp_err_t smbus_send_byte(const smbus_info_t *smbus_info, uint8_t data);

/**
 * @brief Receive a single byte from a slave device.
 * @param[in] smbus_info Pointer to initialised SMBus info instance.
 * @param[out] data Data byte received from slave.
 * @return ESP_OK if successful, ESP_FAIL or ESP_ERR_* if an error occurred.
 */
esp_err_t smbus_receive_byte(const smbus_info_t *smbus_info, uint8_t *data);

/**
 * @brief Write a single byte to a slave device with a command code.
 * @param[in] smbus_info Pointer to initialised SMBus info instance.
 * @param[in] command Device-specific command byte.
 * @param[in] data Data byte to send to slave.
 * @return ESP_OK if successful, ESP_FAIL or ESP_ERR_* if an error occurred.
 */
esp_err_t smbus_write_byte(const smbus_info_t *smbus_info, uint8_t command,
                           uint8_t data);

/**
 * @brief Write a single word (two bytes) to a slave device with a command code.
 *        The least significant byte is transmitted first.
 * @param[in] smbus_info Pointer to initialised SMBus info instance.
 * @param[in] command Device-specific command byte.
 * @param[in] data Data word to send to slave.
 * @return ESP_OK if successful, ESP_FAIL or ESP_ERR_* if an error occurred.
 */
esp_err_t smbus_write_word(const smbus_info_t *smbus_info, uint8_t command,
                           uint16_t data);

/**
 * @brief Read a single byte from a slave device with a command code.
 * @param[in] smbus_info Pointer to initialised SMBus info instance.
 * @param[in] command Device-specific command byte.
 * @param[out] data Data byte received from slave.
 * @return ESP_OK if successful, ESP_FAIL or ESP_ERR_* if an error occurred.
 */
esp_err_t smbus_read_byte(const smbus_info_t *smbus_info, uint8_t command,
                          uint8_t *data);

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

/**
 * @brief Write up to 255 bytes to a slave device with a command code.
 *        This uses a byte count to negotiate the length of the transaction.
 *        The first byte in the data array is transmitted first.
 * @param[in] smbus_info Pointer to initialised SMBus info instance.
 * @param[in] command Device-specific command byte.
 * @param[in] data Data bytes to send to slave.
 * @param[in] len Number of bytes to send to slave.
 * @return ESP_OK if successful, ESP_FAIL or ESP_ERR_* if an error occurred.
 */
esp_err_t smbus_write_block(const smbus_info_t *smbus_info, uint8_t command,
                            uint8_t *data, uint8_t len);

/**
 * @brief Read up to 255 bytes from a slave device with a command code.
 *        This uses a byte count to negotiate the length of the transaction.
 *        The first byte received is placed in the first array location.
 * @param[in] smbus_info Pointer to initialised SMBus info instance.
 * @param[in] command Device-specific command byte.
 * @param[out] data Data bytes received from slave.
 * @param[in/out] len Size of data array, and number of bytes actually received.
 * @return ESP_OK if successful, ESP_FAIL or ESP_ERR_* if an error occurred.
 */
esp_err_t smbus_read_block(const smbus_info_t *smbus_info, uint8_t command,
                           uint8_t *data, uint8_t *len);

/**
 * @brief Write bytes to a slave device with a command code.
 *        No byte count is used - the transaction lasts as long as the master
 * requires. The first byte in the data array is transmitted first. This
 * operation is not defined by the SMBus specification.
 * @param[in] smbus_info Pointer to initialised SMBus info instance.
 * @param[in] command Device-specific command byte.
 * @param[in] data Data bytes to send to slave.
 * @param[in] len Number of bytes to send to slave.
 * @return ESP_OK if successful, ESP_FAIL or ESP_ERR_* if an error occurred.
 */
esp_err_t smbus_i2c_write_block(const smbus_info_t *smbus_info, uint8_t command,
                                uint8_t *data, size_t len);

/**
 * @brief Read bytes from a slave device with a command code (combined format).
 *        No byte count is used - the transaction lasts as long as the master
 * requires. The first byte received is placed in the first array location. This
 * operation is not defined by the SMBus specification.
 * @param[in] smbus_info Pointer to initialised SMBus info instance.
 * @param[in] command Device-specific command byte.
 * @param[out] data Data bytes received from slave.
 * @param[in/out] len Size of data array. If the slave fails to provide
 * sufficient bytes, ESP_ERR_TIMEOUT will be returned.
 * @return ESP_OK if successful, ESP_FAIL or ESP_ERR_* if an error occurred.
 */
esp_err_t smbus_i2c_read_block(const smbus_info_t *smbus_info, uint8_t command,
                               uint8_t *data, size_t len);

typedef enum {
  TSL2561_DEVICE_TYPE_INVALID = 0b1111,   ///< Invalid device
  TSL2561_DEVICE_TYPE_TSL2560CS = 0b0000, ///< TSL2560CS (Chipscale)
  TSL2561_DEVICE_TYPE_TSL2561CS = 0b0001, ///< TSL2561CS (Chipscale)
  TSL2561_DEVICE_TYPE_TSL2560T_FN_CL =
      0b0100, ///< TSL2560T/FN/CL (TMB-6 or Dual Flat No-Lead-6 or ChipLED-6)
  TSL2561_DEVICE_TYPE_TSL2561T_FN_CL =
      0b0101, ///< TSL2561T/FN/CL (TMB-6 or Dual Flat No-Lead-6 or ChipLED-6)
} tsl2561_device_type_t;

/**
 * @brief Enum for supported integration durations.
 * These durations assume the default internal oscillator frequency of 735 kHz.
 */
typedef enum {
  TSL2561_INTEGRATION_TIME_13MS = 0x00,  ///< Integrate over 13.7 milliseconds
  TSL2561_INTEGRATION_TIME_101MS = 0x01, ///< Integrate over 101 milliseconds
  TSL2561_INTEGRATION_TIME_402MS = 0x02, ///< Integrate over 402 milliseconds
} tsl2561_integration_time_t;

/**
 * @brief Enum for supported gain values.
 */
typedef enum {
  TSL2561_GAIN_1X = 0x00,
  TSL2561_GAIN_16X = 0x10,
} tsl2561_gain_t;

typedef uint8_t tsl2561_revision_t; ///< The type of the IC's revision value
typedef uint16_t
    tsl2561_visible_t; ///< The type of a visible light measurement value
typedef uint16_t
    tsl2561_infrared_t; ///< The type of an infrared light measurement value

/**
 * @brief Structure containing information related to the SMBus protocol.
 */
typedef struct {
  bool init;    ///< True if struct has been initialised, otherwise false
  bool powered; ///< True if the device has been powered up
  smbus_info_t *smbus_info; ///< Pointer to associated SMBus info
  tsl2561_device_type_t
      device_type; ///< Detected type of device (Chipscale vs T/FN/CL)
  tsl2561_integration_time_t
      integration_time; ///< Current integration time for measurements
  tsl2561_gain_t gain;  ///< Current gain for measurements
} tsl2561_info_t;

/**
 * @brief Construct a new TSL2561 info instance.
 *        New instance should be initialised before calling other functions.
 * @return Pointer to new device info instance, or NULL if it cannot be created.
 */
tsl2561_info_t *tsl2561_malloc(void);

/**
 * @brief Delete an existing TSL2561 info instance.
 * @param[in,out] tsl2561_info Pointer to TSL2561 info instance that will be
 * freed and set to NULL.
 */
void tsl2561_free(tsl2561_info_t **tsl2561_info);

/**
 * @brief Initialise a TSL2561 info instance with the specified SMBus
 * information.
 * @param[in] tsl2561_info Pointer to TSL2561 info instance.
 * @param[in] smbus_info Pointer to SMBus info instance.
 */
esp_err_t tsl2561_init(tsl2561_info_t *tsl2561_info, smbus_info_t *smbus_info);

/**
 * @brief Retrieve the Device Type ID and Revision number from the device.
 * @param[in] tsl2561_info Pointer to initialised TSL2561 info instance.
 * @param[out] device The retrieved Device Type ID.
 * @param[out] revision The retrieved Device Revision number.
 * @return ESP_OK if successful, ESP_FAIL or ESP_ERR_* if an error occurred.
 */
esp_err_t tsl2561_device_id(const tsl2561_info_t *tsl2561_info,
                            tsl2561_device_type_t *device,
                            tsl2561_revision_t *revision);

/**
 * @brief Set the integration time and gain. These values are set together
 *        as they are programmed via the same register.
 * @param[in] tsl2561_info Pointer to initialised TSL2561 info instance.
 * @param[out] integration_time The integration time to use for the next
 * measurement.
 * @param[out] infrared The gain setting to use for the next measurement.
 * @return ESP_OK if successful, ESP_FAIL or ESP_ERR_* if an error occurred.
 */
esp_err_t tsl2561_set_integration_time_and_gain(
    tsl2561_info_t *tsl2561_info, tsl2561_integration_time_t integration_time,
    tsl2561_gain_t gain);

/**
 * @brief Retrieve a visible and infrared light measurement from the device.
 *        This function will sleep until the integration time has passed.
 * @param[in] tsl2561_info Pointer to initialised TSL2561 info instance.
 * @param[out] visible The resultant visible light measurement.
 * @param[out] infrared The resultant infrared light measurement.
 * @return ESP_OK if successful, ESP_FAIL or ESP_ERR_* if an error occurred.
 */
esp_err_t tsl2561_read(tsl2561_info_t *tsl2561_info, tsl2561_visible_t *visible,
                       tsl2561_infrared_t *infrared);

/**
 * @brief Compute the Lux approximation from a visible and infrared light
 * measurement. The calculation is performed according to the procedure given in
 * the datasheet.
 * @param[in] tsl2561_info Pointer to initialised TSL2561 info instance.
 * @param[in] visible The visible light measurement.
 * @param[in] infrared The infrared light measurement.
 * @return The resulting approximation of the light measurement in Lux.
 */
uint32_t tsl2561_compute_lux(const tsl2561_info_t *tsl2561_info,
                             tsl2561_visible_t visible,
                             tsl2561_infrared_t infrared);

void i2c_master_init(void);
void initialize_tsl2561(void);
void get_sensor_values(void);
#endif // TSL2561_H
