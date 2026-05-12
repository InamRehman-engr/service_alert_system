#ifndef __INA226_H__
#define __INA226_H__

#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "i2c-dev.h"
#include "stdint.h"
#include "time.h"
#include <math.h>
#include <stdio.h>

/**
 * @defgroup Defines
 * @{
 */

#define INA226_CONFIG_REG 0x00
#define INA226_SHUNT_VOLTAGE_REG 0x01
#define INA226_BUS_VOLTAGE_REG 0x02
#define INA226_POWER_REG 0x03
#define INA226_CURRENT_REG 0x04
#define INA226_CALIBRATION_REG 0x05

//  returned by setMaxCurrentShunt
#define INA226_ERR_NONE ESP_OK
#define INA226_ERR_SHUNTVOLTAGE_HIGH 0x8000
#define INA226_ERR_MAXCURRENT_LOW 0x8001
#define INA226_ERR_SHUNT_LOW 0x8002

//  CONFIGURATION MASKS
#define INA226_CONF_RESET_MASK 0x8000
#define INA226_CONF_AVERAGE_MASK 0x0E00
#define INA226_CONF_BUSVC_MASK 0x01C0
#define INA226_CONF_SHUNTVC_MASK 0x0038
#define INA226_CONF_MODE_MASK 0x0007

#define INA226_CONFIG_RESET (1 << 15)
#define INA226_CONFIG_MODE_CONT_SHUNT_BUS (7 << 3)
#define INA226_CONFIG_VBUS_CT_140us (1 << 7)
#define INA226_CONFIG_VSH_CT_140us (1 << 6)

#define MAX_CURRENT 5 * 1.5
#define MAX_VOLTAGE 27 * 1.5
#define MIN_VOLTAGE 21 * 0.5
#define MAX_SHUNT_VOLTAGE 0.1
#define MIN_SHUNT_VOLTAGE 0.001
#define MAX_POWER MAX_CURRENT *MAX_VOLTAGE
#define MIN_POWER 0.01

/** @} */ // End of Defines

/**
 * @brief All data provided by the sensor is stored in this structure.
 */
typedef struct {
  float Power_W;
  float Voltage_V;
  float Current_A;
  float Shunt_Voltage_mV;
} ina226_data;

typedef struct INA226_instance_t INA226_instance_t;

/**
 * @defgroup Instance
 * @{
 */

struct INA226_instance_t {
  i2c_functions I2C;
  uint8_t address;
  ina226_data device_data;

  float _current_LSB;
  float _power_LSB;
  float _shunt;
  float _maxCurrent;

  /**
   * @name Instance Functions
   * @{
   */

  /**
   * This is a function available in the instance to check if the sensor is
   * available.
   * @param instance is the repassing of the instance.
   * @return returns the esp_err_t to verify if task was preformed.
   */
  esp_err_t (*device_available)(INA226_instance_t *instance);
  /**
   * This is a function available in the instance to get latest data from the
   * sensor.
   * @param instance is the repassing of the instance.
   */
  void (*get_device_values)(INA226_instance_t *instance);
  /** @} */ // End of Instance Functions
};
/** @} */ // End of Instance

/**
 * This is the initialization method for the sensor.
 * it's use is to initialize sensors task and functions of the instance.
 * @param instance is like an object which is used for the controlling of INA226
 * IC, multiple instances can be created if multiple IC's are installed in the
 * system.
 */
esp_err_t INA226_init(INA226_instance_t *instance);

#endif