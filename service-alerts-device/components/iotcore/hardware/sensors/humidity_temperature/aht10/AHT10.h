#ifndef __AHT10_H
#define __AHT10_H

// ---------------------------------------------------------------------------
// INCLUDES

#include <stdint.h>

#include "driver/i2c.h"
#include "sdkconfig.h"

// ---------------------------------------------------------------------------
// DEFINES

//
// i2c related values

#define I2C_TIMEOUT_MS 1000

#define ACK_CHECK_EN 0x1
#define ACK_CHECK_DIS 0x0

#define ACK_VAL 0x0
#define NACK_VAL 0x1

#define AHT10_I2C_ADDR 0x38

// ---------------------------------------------------------------------------
// STRUCTURES

struct AHT10_reading {
  float humidity;
  float temperature;
};

// ---------------------------------------------------------------------------
// CONSTANTS

static const uint8_t AHT10_TRIGGER_MEASUREMENT[] = {0xAC, 0x33, 0x00};

// ---------------------------------------------------------------------------
// FUNCTION PROTOTYPES

//
// external

esp_err_t readHumidity(const i2c_port_t i2c_num, float *humidity);

esp_err_t readTemperature(const i2c_port_t i2c_num, float *temperature);

//
// internal

/**
 * perform a sensor reading, and write the value in its correct units to the
 * specified location in memory.
 *
 * @param   i2c_num         the I2C port to read from
 * @param   i2c_command     specify which reading to perform
 * @param   output          the location in memory in which to write the
 *                          resulting value
 * @param   fn              a pointer to the conversion function to use on the
 *                          raw value read from the sensor
 *
 * @returns esp_err_t       the success status of the read
 */
esp_err_t getTemperatureReading(const i2c_port_t num,
                                const uint8_t *i2c_command, float *output,
                                float (*fn)(const uint32_t));

esp_err_t getHumidityReading(const i2c_port_t num, const uint8_t *i2c_command,
                             float *output, float (*fn)(const uint32_t));
/**
 * read the specified number of bytes from the I2C port specified to the
 * provided location in memory.
 *
 * @param   i2c_num         the I2C port to read from
 * @param   i2c_command     a pointer to the location in memory in which to
 *                          write the data to
 * @param   nbytes          the number of bytes to read from the queue
 *
 * @returns esp_err_t       the success status of the read
 */
esp_err_t readResponseBytes(const i2c_port_t i2c_num, uint8_t *output,
                            const size_t nbytes);

/**
 * write the specified number of bytes from the provided location in memory to
 * the I2C port specified.
 *
 * @param   i2c_num         the I2C port to write to
 * @param   i2c_command     a pointer to the location in memory containing the
 *                          data byte(s) to write
 * @param   nbytes          the number of bytes to write to the queue
 *
 * @returns esp_err_t       the success status of the write
 */
esp_err_t writeCommandBytes(const i2c_port_t i2c_num,
                            const uint8_t *i2c_command, const size_t nbytes);

/**
 * Given a 'humidity code' read from the sensor, convert the code to a decimal
 * value representing the relative humidity in percent, and return it.
 *
 * @param   rh_code     the encoded relative humidity reading from the sensor
 *
 * @returns float       the current relative humidity as read by the sensor,
 *                      in percent
 */
float ConvertRawHumidityToPercentage(const uint32_t rh_code);

/**
 * Given a 'temperature code' read from the sensor, convert the code to a
 * decimal value representing the temperature in degrees celsius, and return
 * it.
 *
 * @param   temp_code   the encoded temperature reading from the sensor
 *
 * @returns float       the current temperature as read by the sensor, in
 *                      degrees celsius
 */
float ConvertRawTemperatureToCelcius(const uint32_t temp_code);
int aht10_Read(float *humidity, float *temperature);
void app_i2c_aht10(void (*fun_ptr)(void), uint32_t measurement_time_sec);
#endif