/**
 * @file INA219.h
 * @author   Cowlar Design Studio
 * */
#ifndef _INA219_H_
#define _INA219_H_

#include "driver/i2c.h"
#include "esp_log.h"
#include "sdkconfig.h"
#include <i2c-dev.h>
#include <stdio.h>

#define MAX_CURRENT 5 * 1.5
#define MAX_VOLTAGE 27 * 1.5
#define MIN_VOLTAGE 21 * 0.5
#define MAX_SHUNT_VOLTAGE 0.1
#define MIN_SHUNT_VOLTAGE 0.001
#define MAX_POWER MAX_CURRENT *MAX_VOLTAGE
#define MIN_POWER 0.01

/**
 * @defgroup Defines addresses & mask
 * @{
 */

#define INA219_READ (0x01)

/**
 * @name Configuration Register
 * @{
 */

#define INA219_REG_CONFIG (0x00)

/** @} */ // Configuration Register

/**
 * @name Reset Mask
 * @{
 */

#define INA219_CONFIG_RESET (0x8000) // Reset Bit

/** @} */ // Configuration Register

/**
 * @name Reset Mask
 * @{
 */

#define INA219_CONFIG_BVOLTAGERANGE_MASK (0x2000) // Bus Voltage Range Mask
#define INA219_CONFIG_BVOLTAGERANGE_16V (0x0000)  // 0-16V Range
#define INA219_CONFIG_BVOLTAGERANGE_32V (0x2000)  // 0-32V Range
/** @} */                                         // End of Bus Voltage

/**
 * @name Gain Mask
 * @{
 */

#define INA219_CONFIG_GAIN_MASK (0x1800)    // Gain Mask
#define INA219_CONFIG_GAIN_1_40MV (0x0000)  // Gain 1, 40mV Range
#define INA219_CONFIG_GAIN_2_80MV (0x0800)  // Gain 2, 80mV Range
#define INA219_CONFIG_GAIN_4_160MV (0x1000) // Gain 4, 160mV Range
#define INA219_CONFIG_GAIN_8_320MV (0x1800) // Gain 8, 320mV Range

/** @} */ // End of Gain Mask

/**
 * @name Bus ADC Resolution
 * @{
 */

#define INA219_CONFIG_BADCRES_MASK (0x0780)  // Bus ADC Resolution Mask
#define INA219_CONFIG_BADCRES_9BIT (0x0080)  // 9-bit bus res = 0..511
#define INA219_CONFIG_BADCRES_10BIT (0x0100) // 10-bit bus res = 0..1023
#define INA219_CONFIG_BADCRES_11BIT (0x0200) // 11-bit bus res = 0..2047
#define INA219_CONFIG_BADCRES_12BIT (0x0400) // 12-bit bus res = 0..4097

/** @} */ // End of ADC Resolution

/**
 * @name SADCRES settings & Shunt ADC Resolution
 * @{
 */

#define INA219_CONFIG_SADCRES_MASK                                             \
  (0x0078) // Shunt ADC Resolution and Averaging Mask
#define INA219_CONFIG_SADCRES_9BIT_1S_84US (0x0000)   // 1 x 9-bit shunt sample
#define INA219_CONFIG_SADCRES_10BIT_1S_148US (0x0008) // 1 x 10-bit shunt sample
#define INA219_CONFIG_SADCRES_11BIT_1S_276US (0x0010) // 1 x 11-bit shunt sample
#define INA219_CONFIG_SADCRES_12BIT_1S_532US (0x0018) // 1 x 12-bit shunt sample
#define INA219_CONFIG_SADCRES_12BIT_2S_1060US                                  \
  (0x0048) // 2 x 12-bit shunt samples averaged together
#define INA219_CONFIG_SADCRES_12BIT_4S_2130US                                  \
  (0x0050) // 4 x 12-bit shunt samples averaged together
#define INA219_CONFIG_SADCRES_12BIT_8S_4260US                                  \
  (0x0058) // 8 x 12-bit shunt samples averaged together
#define INA219_CONFIG_SADCRES_12BIT_16S_8510US                                 \
  (0x0060) // 16 x 12-bit shunt samples averaged together
#define INA219_CONFIG_SADCRES_12BIT_32S_17MS                                   \
  (0x0068) // 32 x 12-bit shunt samples averaged together
#define INA219_CONFIG_SADCRES_12BIT_64S_34MS                                   \
  (0x0070) // 64 x 12-bit shunt samples averaged together
#define INA219_CONFIG_SADCRES_12BIT_128S_69MS                                  \
  (0x0078) // 128 x 12-bit shunt samples averaged together

/** @} */ // End of SADCRES settings & Shunt ADC Resolution

/**
 * @name MODE
 * @{
 */
#define INA219_CONFIG_MODE_MASK (0x0007) // Operating Mode Mask
#define INA219_CONFIG_MODE_POWERDOWN (0x0000)
#define INA219_CONFIG_MODE_SVOLT_TRIGGERED (0x0001)
#define INA219_CONFIG_MODE_BVOLT_TRIGGERED (0x0002)
#define INA219_CONFIG_MODE_SANDBVOLT_TRIGGERED (0x0003)
#define INA219_CONFIG_MODE_ADCOFF (0x0004)
#define INA219_CONFIG_MODE_SVOLT_CONTINUOUS (0x0005)
#define INA219_CONFIG_MODE_BVOLT_CONTINUOUS (0x0006)
#define INA219_CONFIG_MODE_SANDBVOLT_CONTINUOUS (0x0007)

/** @} */ // End of MODE

/**
 * @name Shunt Voltage Register
 * @{
 */

#define INA219_REG_SHUNTVOLTAGE (0x01)
/** @} */ // End of Shunt Voltage Register

/**
 * @name Bus Voltage Register
 * @{
 */

#define INA219_REG_BUSVOLTAGE (0x02)
/** @} */ // End of Bus Voltage Register

/**
 * @name Power Register
 * @{
 */

#define INA219_REG_POWER (0x03)
/** @} */ // End of Power Register

/**
 * @name Current Register
 * @{
 */

#define INA219_REG_CURRENT (0x04)
/** @} */ // End of Current Register

/**
 * @name Calibration Register
 * @{
 */

#define INA219_REG_CALIBRATION (0x05)
/** @} */ // End of Calibration Register
/** @} */ // Defines addresses & mask

typedef enum {
  CAL_32V_2A,
  CAL_32V_1A,
  CAL_16V_1A_100mOhm,
  CAL_16V_400mA,
  CAL_16V_40mA_1ohm,
  CAL_16V_40mA_4mohm,
  CAL_16V_320mA_1ohm
} calibration_Type;

typedef struct INA219_instance_t INA219_instance_t;

/**
 * @brief All data provided by the sensor is stored in this structure.
 */
typedef struct {
  float Power_mW;
  float Current_mA;
  float Voltage_V;
  float ShuntVoltage_mV;
} ina219_data;

/**
 * @defgroup Instance
 * @{
 */

struct INA219_instance_t {
  i2c_functions I2C;
  uint8_t address;
  calibration_Type calibration;
  uint32_t ina219_calValue;
  ina219_data device_data;

  // The following multipliers are used to convert raw current and power
  // values to mA and mW, taking into account the current config settings
  float ina219_currentDivider_mA;
  float ina219_powerDivider_mW;

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
  esp_err_t (*device_available)(INA219_instance_t *instance);

  /**
   * This is a function available in the instance to get latest data from the
   * sensor.
   * @param instance is the repassing of the instance.
   */
  void (*get_device_values)(INA219_instance_t *instance);
  /** @} */ // End of Instance Functions
};
/** @} */ // End of Instance

/**
 * This is the initialization method for the sensor.
 * it's use is to initialize sensors task and functions of the instance.
 * @param instance is like an object which is used for the controlling of INA219
 * IC, multiple instances can be created if multiple IC's are installed in the
 * system.
 */
esp_err_t INA219_Init(INA219_instance_t *instance);

#endif // _INA219_H_
