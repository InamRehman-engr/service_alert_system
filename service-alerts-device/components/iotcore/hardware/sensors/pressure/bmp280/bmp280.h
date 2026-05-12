#ifndef _bmp_280_h
#define _bmp_280_h

#include "esp_err.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "math.h"
#include <i2c-dev.h>
#include <spi_master_dev.h>

#define BMP280_CHIPID 0x58

/**
 * @brief BMP280 select communication interface options.
 */
typedef enum {
  BMP280_I2C,
  BMP280_SPI,
} bmp280_interface;

/**
 * @brief BMP280 I2C address options.
 */
typedef enum {
  BMP280_SDO_HIGH_ADDRESS = 0x77,
  BMP280_SDO_LOW_ADDRESS = 0x76,
} bmp280_address;

/**
 * @brief BMP280 operation modes.
 */
typedef enum {
  BMP280_MODE_SLEEP = 0x00,
  BMP280_MODE_FORCED = 0x01,
  BMP280_MODE_NORMAL = 0x03,
  BMP280_MODE_SOFT_RESET_CODE = 0xB6
} bmp280_mode;

/**
 * @brief BMP280 sampling rate options for temperature and pressure.
 */
typedef enum {
  BMP280_SAMPLING_NONE = 0x00,
  BMP280_SAMPLING_X1 = 0x01,
  BMP280_SAMPLING_X2 = 0x02,
  BMP280_SAMPLING_X4 = 0x03,
  BMP280_SAMPLING_X8 = 0x04,
  BMP280_SAMPLING_X16 = 0x05
} bmp280_sampling;

/**
 * @brief Enumeration for BMP280 filter coefficient options.
 */
typedef enum {
  BMP280_FILTER_OFF = 0x00,
  BMP280_FILTER_X2 = 0x01,
  BMP280_FILTER_X4 = 0x02,
  BMP280_FILTER_X8 = 0x03,
  BMP280_FILTER_X16 = 0x04
} bmp280_filter;

/**
 * @brief Enumeration for BMP280 standby duration options.
 */
typedef enum {
  BMP280_STANDBY_MS_1 = 0x00,
  BMP280_STANDBY_MS_63 = 0x01,
  BMP280_STANDBY_MS_125 = 0x02,
  BMP280_STANDBY_MS_250 = 0x03,
  BMP280_STANDBY_MS_500 = 0x04,
  BMP280_STANDBY_MS_1000 = 0x05,
  BMP280_STANDBY_MS_2000 = 0x06,
  BMP280_STANDBY_MS_4000 = 0x07
} bmp280_standby_duration;

/**
 * @brief BMP280 register addresses.
 */
enum {
  BMP280_REGISTER_DIG_T1 = 0x88,
  BMP280_REGISTER_DIG_T2 = 0x8A,
  BMP280_REGISTER_DIG_T3 = 0x8C,
  BMP280_REGISTER_DIG_P1 = 0x8E,
  BMP280_REGISTER_DIG_P2 = 0x90,
  BMP280_REGISTER_DIG_P3 = 0x92,
  BMP280_REGISTER_DIG_P4 = 0x94,
  BMP280_REGISTER_DIG_P5 = 0x96,
  BMP280_REGISTER_DIG_P6 = 0x98,
  BMP280_REGISTER_DIG_P7 = 0x9A,
  BMP280_REGISTER_DIG_P8 = 0x9C,
  BMP280_REGISTER_DIG_P9 = 0x9E,
  BMP280_REGISTER_CHIPID = 0xD0,
  BMP280_REGISTER_VERSION = 0xD1,
  BMP280_REGISTER_SOFTRESET = 0xE0,
  BMP280_REGISTER_CAL26 = 0xE1,
  BMP280_REGISTER_STATUS = 0xF3,
  BMP280_REGISTER_CONTROL = 0xF4,
  BMP280_REGISTER_CONFIG = 0xF5,
  BMP280_REGISTER_PRESSUREDATA = 0xF7,
  BMP280_REGISTER_TEMPDATA = 0xFA,
};

/**
 * @brief Calibration data structure for BMP280 sensor.
 */
typedef struct {
  uint16_t dig_T1;
  int16_t dig_T2;
  int16_t dig_T3;
  uint16_t dig_P1;
  int16_t dig_P2;
  int16_t dig_P3;
  int16_t dig_P4;
  int16_t dig_P5;
  int16_t dig_P6;
  int16_t dig_P7;
  int16_t dig_P8;
  int16_t dig_P9;
} bmp280_calib_data;

/**
 * @brief Configuration structure for BMP280 sensor.
 */
typedef struct {
  bmp280_address address;
  bmp280_interface interface;
  bmp280_mode mode;
  bmp280_sampling tempSampling;
  bmp280_sampling pressSampling;
  bmp280_filter filter;
  bmp280_standby_duration duration;
} bmp280_config_t;

/**
 * @brief BMP280 handle structure.
 */
typedef struct {
  union {
    spi_master_functions *spi;
    i2c_functions *i2c;
  } bmp280_interface;

  bmp280_config_t config;
  bmp280_calib_data bmp280_calib;

  float temperature;
  float pressure;

  /**
   * @brief Function pointer to retrieve temperature and pressure from BMP280.
   * @param handle Pointer to the BMP280 handle.
   * @return Pressure in hPa.
   */
  esp_err_t (*getData)(struct bmp280_handle_t *handle);

  /**
   * @brief Function pointer to trigger a forced measurement.
   * @return ESP_OK on success, ESP_FAIL on failure.
   */
  esp_err_t (*takeForcedMeasurement)(void);
} bmp280_handle_t;

/**
 * @brief Initialize BMP280 sensor.
 *
 * @param handle Pointer to the BMP280 handle.
 * @param config Pointer to the BMP280 configuration.
 * @return ESP_OK on success, ESP_FAIL on failure.
 *
 * Pass NULL in config to initialize sensor with default config. Stores config
 * in handle.
 */
esp_err_t bmp280_init(bmp280_handle_t *handle, bmp280_config_t *config);

#endif //_bmp_280_h