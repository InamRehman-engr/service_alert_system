#include "bmp280.h"

char *TAG = "BME280";
uint8_t sensorID = 0;
int32_t t_fine;

bmp280_config_t bmp280_default_config = {
    .interface = BMP280_I2C,
    .mode = BMP280_MODE_NORMAL,
    .address = BMP280_SDO_LOW_ADDRESS,
    .tempSampling = BMP280_SAMPLING_X16,
    .pressSampling = BMP280_SAMPLING_X16,
    .filter = BMP280_FILTER_OFF,
    .duration = BMP280_STANDBY_MS_1000,
};
typedef struct {
  unsigned int t_sb;
  unsigned int filter;
  unsigned int none;
  unsigned int spi3w_en;
} config_struct;

typedef struct {
  unsigned int osrs_t;
  unsigned int osrs_p;
  unsigned int mode;
} ctrl_meas_struct;

esp_err_t write8bit(bmp280_handle_t *handle, uint8_t reg, uint8_t value) {
  uint8_t buffer[2];
  buffer[1] = value;
  if (handle->config.interface == BMP280_I2C) {
    buffer[0] = reg;
    handle->bmp280_interface.i2c->i2c_send(
        handle->config.address, buffer, sizeof(buffer),
        handle->bmp280_interface.i2c->device);
    vTaskDelay(pdMS_TO_TICKS(10));
  } else if (handle->config.interface == BMP280_SPI) {
    buffer[0] = reg & ~0x80;
    handle->bmp280_interface.spi->send(&handle->bmp280_interface.spi, 0, 0,
                                       buffer, sizeof(buffer));
  }
  return ESP_OK;
}

uint8_t read8bit(bmp280_handle_t *handle, uint8_t reg) {
  uint8_t txBuffer[1];
  uint8_t rxBuffer[1];
  if (handle->config.interface == BMP280_I2C) {
    txBuffer[0] = reg;
    handle->bmp280_interface.i2c->i2c_send_receive(
        handle->config.address, txBuffer, sizeof(txBuffer), rxBuffer,
        sizeof(rxBuffer), handle->bmp280_interface.i2c->device);
  } else if (handle->config.interface == BMP280_SPI) {
    txBuffer[0] = reg | 0x80;
    handle->bmp280_interface.spi->send(&handle->bmp280_interface.spi, 0, 0,
                                       txBuffer, sizeof(txBuffer));
    // vTaskDelay(pdMS_TO_TICKS(10));
    handle->bmp280_interface.spi->receive(&handle->bmp280_interface.spi, 0, 0,
                                          rxBuffer, sizeof(rxBuffer));
  }
  return rxBuffer[0];
}

uint16_t read16bit(bmp280_handle_t *handle, uint8_t reg) {
  uint8_t txBuffer[1];
  uint8_t rxBuffer[2];
  uint16_t tempBuffer;
  if (handle->config.interface == BMP280_I2C) {
    txBuffer[0] = reg;
    handle->bmp280_interface.i2c->i2c_send_receive(
        handle->config.address, txBuffer, sizeof(txBuffer), rxBuffer,
        sizeof(rxBuffer), handle->bmp280_interface.i2c->device);
  } else if (handle->config.interface == BMP280_SPI) {
    txBuffer[0] = reg | 0x80;
    handle->bmp280_interface.spi->send(&handle->bmp280_interface.spi, 0, 0,
                                       txBuffer, sizeof(txBuffer));
    // vTaskDelay(pdMS_TO_TICKS(10));
    handle->bmp280_interface.spi->receive(&handle->bmp280_interface.spi, 0, 0,
                                          rxBuffer, sizeof(rxBuffer));
  }

  tempBuffer = rxBuffer[0] << 8 | rxBuffer[1];
  return tempBuffer;
}

uint16_t read16bit_LE(bmp280_handle_t *handle, uint8_t reg) {
  uint16_t tempBuffer = read16bit(handle, reg);
  return (tempBuffer >> 8) | (tempBuffer << 8);
}

int16_t readsigned16bit(bmp280_handle_t *handle, uint8_t reg) {
  return (int16_t)(read16bit(handle, reg));
}

int16_t readsigned16bit_LE(bmp280_handle_t *handle, uint8_t reg) {
  return (int16_t)(read16bit_LE(handle, reg));
}

uint32_t read24bit(bmp280_handle_t *handle, uint8_t reg) {
  uint8_t txBuffer[1];
  uint8_t rxBuffer[3];
  uint32_t tempBuffer;
  if (handle->config.interface == BMP280_I2C) {
    txBuffer[0] = reg;
    handle->bmp280_interface.i2c->i2c_send_receive(
        handle->config.address, txBuffer, sizeof(txBuffer), rxBuffer,
        sizeof(rxBuffer), handle->bmp280_interface.i2c->device);
  } else if (handle->config.interface == BMP280_SPI) {
    txBuffer[0] = reg | 0x80;
    handle->bmp280_interface.spi->send(&handle->bmp280_interface.spi, 0, 0,
                                       txBuffer, sizeof(txBuffer));
    // vTaskDelay(pdMS_TO_TICKS(10));
    handle->bmp280_interface.spi->receive(&handle->bmp280_interface.spi, 0, 0,
                                          rxBuffer, sizeof(rxBuffer));
  }

  tempBuffer = rxBuffer[0] << 16 | rxBuffer[1] << 8 | rxBuffer[2];
  return tempBuffer;
}

void getCalibrationData(bmp280_handle_t *handle) {
  handle->bmp280_calib.dig_T1 = read16bit_LE(handle, BMP280_REGISTER_DIG_T1);
  handle->bmp280_calib.dig_T2 =
      readsigned16bit_LE(handle, BMP280_REGISTER_DIG_T2);
  handle->bmp280_calib.dig_T3 =
      readsigned16bit_LE(handle, BMP280_REGISTER_DIG_T3);

  handle->bmp280_calib.dig_P1 = read16bit_LE(handle, BMP280_REGISTER_DIG_P1);
  handle->bmp280_calib.dig_P2 =
      readsigned16bit_LE(handle, BMP280_REGISTER_DIG_P2);
  handle->bmp280_calib.dig_P3 =
      readsigned16bit_LE(handle, BMP280_REGISTER_DIG_P3);
  handle->bmp280_calib.dig_P4 =
      readsigned16bit_LE(handle, BMP280_REGISTER_DIG_P4);
  handle->bmp280_calib.dig_P5 =
      readsigned16bit_LE(handle, BMP280_REGISTER_DIG_P5);
  handle->bmp280_calib.dig_P6 =
      readsigned16bit_LE(handle, BMP280_REGISTER_DIG_P6);
  handle->bmp280_calib.dig_P7 =
      readsigned16bit_LE(handle, BMP280_REGISTER_DIG_P7);
  handle->bmp280_calib.dig_P8 =
      readsigned16bit_LE(handle, BMP280_REGISTER_DIG_P8);
  handle->bmp280_calib.dig_P9 =
      readsigned16bit_LE(handle, BMP280_REGISTER_DIG_P9);

  ESP_LOGD(TAG, "dig_T1: 0x%X", handle->bmp280_calib.dig_T1);
  ESP_LOGD(TAG, "dig_T2: 0x%X", handle->bmp280_calib.dig_T2);
  ESP_LOGD(TAG, "dig_T3: 0x%X", handle->bmp280_calib.dig_T3);

  ESP_LOGD(TAG, "dig_P1: 0x%X", handle->bmp280_calib.dig_P1);
  ESP_LOGD(TAG, "dig_P2: 0x%X", handle->bmp280_calib.dig_P2);
  ESP_LOGD(TAG, "dig_P3: 0x%X", handle->bmp280_calib.dig_P3);
  ESP_LOGD(TAG, "dig_P4: 0x%X", handle->bmp280_calib.dig_P4);
  ESP_LOGD(TAG, "dig_P5: 0x%X", handle->bmp280_calib.dig_P5);
  ESP_LOGD(TAG, "dig_P6: 0x%X", handle->bmp280_calib.dig_P6);
  ESP_LOGD(TAG, "dig_P7: 0x%X", handle->bmp280_calib.dig_P7);
  ESP_LOGD(TAG, "dig_P8: 0x%X", handle->bmp280_calib.dig_P8);
  ESP_LOGD(TAG, "dig_P9: 0x%X", handle->bmp280_calib.dig_P9);
}

float getTemperature(bmp280_handle_t *handle) {
  int32_t var1, var2;
  if (!sensorID) {
    return ESP_FAIL;
  }

  int32_t adc_T = read24bit(handle, BMP280_REGISTER_TEMPDATA);
  adc_T >>= 4;

  var1 = ((((adc_T >> 3) - ((int32_t)handle->bmp280_calib.dig_T1 << 1))) *
          ((int32_t)handle->bmp280_calib.dig_T2)) >>
         11;

  var2 = (((((adc_T >> 4) - ((int32_t)handle->bmp280_calib.dig_T1)) *
            ((adc_T >> 4) - ((int32_t)handle->bmp280_calib.dig_T1))) >>
           12) *
          ((int32_t)handle->bmp280_calib.dig_T3)) >>
         14;

  t_fine = var1 + var2;

  float T = (t_fine * 5 + 128) >> 8;
  return T / 100;
}

esp_err_t getData(bmp280_handle_t *handle) {
  int64_t var1, var2, p;
  if (!sensorID) {
    return ESP_FAIL;
  }

  handle->temperature = getTemperature(handle);

  int32_t adc_P = read24bit(handle, BMP280_REGISTER_PRESSUREDATA);
  adc_P >>= 4;

  var1 = ((int64_t)t_fine) - 128000;
  var2 = var1 * var1 * (int64_t)handle->bmp280_calib.dig_P6;
  var2 = var2 + ((var1 * (int64_t)handle->bmp280_calib.dig_P5) << 17);
  var2 = var2 + (((int64_t)handle->bmp280_calib.dig_P4) << 35);
  var1 = ((var1 * var1 * (int64_t)handle->bmp280_calib.dig_P3) >> 8) +
         ((var1 * (int64_t)handle->bmp280_calib.dig_P2) << 12);
  var1 = (((((int64_t)1) << 47) + var1)) *
             ((int64_t)handle->bmp280_calib.dig_P1) >>
         33;

  if (var1 == 0) {
    return 0;
  }
  p = 1048576 - adc_P;
  p = (((p << 31) - var2) * 3125) / var1;
  var1 = (((int64_t)handle->bmp280_calib.dig_P9) * (p >> 13) * (p >> 13)) >> 25;
  var2 = (((int64_t)handle->bmp280_calib.dig_P8) * p) >> 19;

  p = ((p + var1 + var2) >> 8) + (((int64_t)handle->bmp280_calib.dig_P7) << 4);
  handle->pressure = (float)p / 256;
  return ESP_OK;
}

esp_err_t bmp280_init(bmp280_handle_t *handle, bmp280_config_t *config) {
  config_struct ctrl_config;
  ctrl_meas_struct ctrl_meas;

  handle->getData = getData;

  if (config == NULL) {
    config = &bmp280_default_config;
  }
  handle->config = *config;

  sensorID = read8bit(handle, BMP280_REGISTER_CHIPID);
  if (sensorID != BMP280_CHIPID) {
    ESP_LOGE(TAG, "Sensor init failed. Sensor ID: %d\n", sensorID);
    return ESP_FAIL;
  } else {
    ESP_LOGD(TAG, "Sensor init successful. Sensor ID: %d\n", sensorID);

    getCalibrationData(handle);

    ctrl_meas.mode = config->mode;
    ctrl_meas.osrs_t = config->tempSampling;
    ctrl_meas.osrs_p = config->pressSampling;
    ctrl_config.filter = config->filter;
    ctrl_config.t_sb = config->duration;

    write8bit(handle, BMP280_REGISTER_CONFIG,
              ctrl_config.t_sb << 5 | ctrl_config.filter << 2);
    write8bit(handle, BMP280_REGISTER_CONTROL,
              ctrl_meas.osrs_t << 5 | ctrl_meas.osrs_p << 2 | ctrl_meas.mode);
    vTaskDelay(pdMS_TO_TICKS(100));
    return ESP_OK;
  }
}