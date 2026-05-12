#include "AS5045.h"
#include "esp_log.h"
#include <math.h>

esp_err_t as5045_encoder_data(spi_master_functions *spi_device_handle,
                              float *angle, bool *OCF, bool *COF, bool *LIN,
                              bool *MAG_INC, bool *MAG_DEC, bool *EVEN_PAR) {
  uint8_t raw_data[4] = {0};
  esp_err_t error;
  if (spi_device_handle != NULL) {
    error = spi_device_handle->receive(spi_device_handle, 0, 0, raw_data, 3);
    if (error != ESP_OK) {
      return error;
    }

  } else {
    ESP_LOGE("AS5045 Encoder", "SPI device handle empty")
    return ESP_FAIL;
  }

  // Extracting angular information (first 12 bits)
  uint16_t angular_info = ((uint16_t)raw_data[0] << 4) | (raw_data[1] >> 4);
  *angle = (float)angular_info / 11.375;

  // Extracting system information
  *OCF = (raw_data[1] >> 3) & 0x01;
  *COF = (raw_data[1] >> 2) & 0x01;
  *LIN = (raw_data[1] >> 1) & 0x01;
  *MAG_INC = raw_data[1] & 0x01;
  *MAG_DEC = (raw_data[2] >> 7) & 0x01;
  *EVEN_PAR = (raw_data[2] >> 6) & 0x01;
  return error;
}

esp_err_t as5045_encoder_configurations(spi_master_functions *spi_device_handle,
                                        float angle, bool CCW, bool PWM_DIS,
                                        bool MAG_COMP_EN, bool PWM_HALF_EN) {
  // Todo: this needs to be tested.
  if (spi_device_handle == NULL) {
    ESP_LOGE("AS5045 Encoder", "SPI device handle empty");
    return ESP_FAIL;
  }

  uint8_t data[2] = {0};
  uint16_t raw_angler = round(angle * 11.375);
  data[0] = data[0] & ((uint8_t)CCW << 7);
  data[0] = data[0] & (raw_angler >> 5);
  data[1] = data[1] & ((raw_angler & 0x1F) << 3);
  data[1] = data[1] & (PWM_DIS << 2);
  data[1] = data[1] & (MAG_COMP_EN << 1);
  data[1] = data[1] & (PWM_HALF_EN);
  return spi_device_handle->send(spi_device_handle, 0, 0, data, 2);
}