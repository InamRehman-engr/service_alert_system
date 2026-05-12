#include "ina226.h"
#include "sdkconfig.h"

static const char *TAG = "INA226";

static esp_err_t
_writeRegister(INA226_instance_t *instance, uint8_t reg,
               uint16_t value) // careful with writing 16 bit values. Platforms
                               // might require you to convert to 2 8 bit values
{
  uint8_t values[3];
  values[0] = reg;
  values[1] = (value >> 8) & 0xFF;
  values[2] = (value & 0xFF);
  esp_err_t ret = instance->I2C.i2c_send(instance->address, values, 3,
                                         instance->I2C.device);
  return ret;
}
static uint16_t _readRegister(INA226_instance_t *instance, uint8_t reg) {
  uint8_t data[2];
  if (instance->I2C.i2c_send_receive(instance->address, &reg, 1, data, 2,
                                     instance->I2C.device) ==
      ESP_OK) // we expect to read 16 bit values
    return (((uint16_t)data[0] << 8) | (uint16_t)data[1]);
  ESP_LOGE(TAG, "I2C Reading Error INA226");
  return 0;
}
// c functions

/*
Returns true if device is available on i2c bus
*/

// User side implementation functions
esp_err_t INA226_device_available(INA226_instance_t *instance) {
  return instance->I2C.device_available(instance->address,
                                        instance->I2C.device);
}

esp_err_t INA226_calibrate(INA226_instance_t *instance) {
  esp_err_t err = ESP_FAIL;
  uint16_t cal =
      (uint16_t)((0.00512) / (instance->_current_LSB * instance->_shunt));
  vTaskDelay(pdMS_TO_TICKS(100));
  err = _writeRegister(instance, 0x00, 0x4127);
  ESP_LOGI(TAG, "Calibration: %.04f A, %.04f Ohm, 0x%04x",
           instance->_maxCurrent, instance->_shunt, cal);
  err = _writeRegister(instance, INA226_CALIBRATION_REG, cal);
  return err;
}

void INA226_get_device_values(INA226_instance_t *instance) {
  ina226_data data = {0};
  data.Shunt_Voltage_mV =
      _readRegister(instance, INA226_SHUNT_VOLTAGE_REG) * 2.5e-6;
  data.Power_W =
      _readRegister(instance, INA226_POWER_REG) * instance->_power_LSB;
  data.Voltage_V = _readRegister(instance, INA226_BUS_VOLTAGE_REG) * 1.25e-3;
  data.Current_A =
      _readRegister(instance, INA226_CURRENT_REG) * instance->_current_LSB;
  if (data.Current_A < MAX_CURRENT || data.Current_A > 0) {
    instance->device_data.Current_A = data.Current_A;
  }
  if (data.Voltage_V < MAX_VOLTAGE || data.Voltage_V > MIN_VOLTAGE) {
    instance->device_data.Voltage_V = data.Voltage_V;
  }
  if (data.Shunt_Voltage_mV < MAX_SHUNT_VOLTAGE ||
      data.Shunt_Voltage_mV > MIN_SHUNT_VOLTAGE) {
    instance->device_data.Shunt_Voltage_mV = data.Shunt_Voltage_mV;
  }
  if (data.Power_W < MAX_POWER || data.Power_W > MIN_POWER) {
    instance->device_data.Power_W = data.Power_W;
  }
}

esp_err_t INA226_init(INA226_instance_t *instance) {
  esp_err_t err = ESP_FAIL;
  if (instance->_maxCurrent == 0 || instance->_shunt == 0 ||
      instance->address == 0) {
    ESP_LOGE(TAG, "Max current, shunt value or device address is 0");
    return ESP_ERR_INVALID_ARG;
  }
  instance->_current_LSB = (instance->_maxCurrent / pow(2, 15));
  instance->_power_LSB = instance->_current_LSB * 25;
  err = INA226_calibrate(instance);
  instance->device_available = INA226_device_available;
  instance->get_device_values = INA226_get_device_values;
  return err;
}