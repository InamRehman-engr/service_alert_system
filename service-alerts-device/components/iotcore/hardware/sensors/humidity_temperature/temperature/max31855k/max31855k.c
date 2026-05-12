#include "max31855k.h"

static const char *TAG = "MAX31855K";

float thermocoupleTemperature(int32_t data) {
  if (data & 0x80000000) {
    data = 0xFFFFC000 | ((data >> 18) & 0x00003FFF);
  } else {
    data >>= 18;
  }
  float extTemp = data;
  extTemp *= 0.25;
  return extTemp;
}

float junctionTemperature(int32_t data) {
  data >>= 4;
  float intTemp = data & 0x7FF;
  if (data & 0x800) {
    int16_t tmp = 0xF800 | (data & 0x7FF);
    intTemp = tmp;
  }
  intTemp *= 0.0625;
  return intTemp;
}

/*
    Junction Temperature = Internal Reference temperature of the MAX31855K IC.
    Thermocouple Temperature = Temperature measure by the thermocouple.

    IMPORTANT: For actual temperature in Celsius, use Temperature parameter.
*/
esp_err_t max31855k_getData(max31855k_handle_t *handle) {
  uint8_t rxBuffer[4] = {0};
  if (handle->max31855k.receive(&handle->max31855k, 0, 0, rxBuffer,
                                sizeof(rxBuffer)) == ESP_OK) {
    int32_t tempRxBuffer = 0;
    tempRxBuffer = (rxBuffer[0] << 24) | (rxBuffer[1] << 16) |
                   (rxBuffer[2] << 8) | rxBuffer[3];
    handle->thermocoupleTemperature = thermocoupleTemperature(tempRxBuffer);
    handle->junctionTemperature = junctionTemperature(tempRxBuffer);
    handle->temperature = thermocoupleTemperature(tempRxBuffer) +
                          junctionTemperature(tempRxBuffer);
    handle->faultBit = (tempRxBuffer & 0x10000) >> 16;
    handle->shortCircuitHighBit = (tempRxBuffer & 4) >> 2;
    handle->shortCircuitLowBit = (tempRxBuffer & 2) >> 1;
    handle->openCircuitBit = (tempRxBuffer & 1);
    return ESP_OK;
  } else {
    ESP_LOGE(TAG, "SPI Read error");
    return ESP_FAIL;
  }
}