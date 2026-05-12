
#include "driver/i2c.h"
#include "esp_log.h"
#include "sdkconfig.h"
#include "th_i2c.h"
#include <stdio.h>

static const char *TAG = "i2c-shtc3";

#define LSB(value) ((uint8_t)(value & 0x00ff)) //(value<<8)>>8
#define MSB(value) ((uint8_t)(value >> 8))
#define GET_BIT(number, bit) ((number >> bit) & 1)
#define decode_2Byte_Parameter(high_byte, low_byte)                            \
  ((uint16_t)((high_byte << 8 & 0xff00) | (low_byte & 0x00ff)))

#define CRC_POLYNOMIAL 0x131
#define SENSOR_PRODUCT_CODE 0b000111
#define SENSOR_PRODUCT_CODE_MASK 0b111111

#define I2C_MASTER_FREQ_HZ 100000 /*!< I2C master clock frequency */

typedef enum {
  READ_ID = 0xEFC8,       // command: read ID register
  SOFT_RESET = 0x805D,    // soft reset
  SLEEP = 0xB098,         // sleep
  WAKEUP = 0x3517,        // wakeup
  T_RH_POLLING = 0x7866,  // read T first, clock stretching disabled
  T_RH_CLOCKSTR = 0x7CA2, // read T first, clock stretching enabled
  RH_T_POLLING = 0x58E0,  // read RH first, clock stretching disabled
  RH_T_CLOCKSTR = 0x5C24  // read RH first, clock stretching enabled
} shtc3_Commands;

typedef enum {
  // SENSOR_FAIL=0X00,
  SENSOR_OK = 0X01,
  SENSOR_WAKEUP_FAIL = 0x02,
  SENSOR_TEMP_HUMI_READ_FAIL = 0x03,
  SENSOR_READ_ID_FAIL = 0x04,
  SENSOR_SOFT_RST_FAIL = 0x05,
  SENSOR_CRC_ERR = 0x06,
} shtc3_err;

#define SHTC3_SENSOR_ADDR 0x70
#define WRITE_BIT 0       /*!< I2C master write */
#define READ_BIT 1        /*!< I2C master read */
#define ACK_CHECK_EN 0x1  /*!< I2C master will check ack from slave*/
#define ACK_CHECK_DIS 0x0 /*!< I2C master will not check ack from slave */
#define ACK_VAL 0x0       /*!< I2C ack value */
#define NACK_VAL 0x1      /*!< I2C nack value */

static float Humidity = 0;
static float Temperature = 0;
SemaphoreHandle_t print_mux = NULL;

static float inline SHTC3_CalcTemperature(uint16_t rawValue) {
  return 175 * (float)rawValue / 65536.0f - 45.0f;
}

static float inline SHTC3_CalcHumidity(uint16_t rawValue) {
  return 100 * (float)rawValue / 65536.0f;
}
static esp_err_t SHTC3_WakeupCommand(i2c_port_t i2c_num) {
  esp_err_t wakeup_err;

  i2c_cmd_handle_t cmd = i2c_cmd_link_create();
  wakeup_err = i2c_master_start(cmd);
  if (wakeup_err == ESP_OK) {
    wakeup_err = i2c_master_write_byte(cmd, SHTC3_SENSOR_ADDR << 1 | WRITE_BIT,
                                       ACK_CHECK_EN);
    wakeup_err |= i2c_master_write_byte(cmd, MSB(WAKEUP), ACK_CHECK_EN);
    wakeup_err |= i2c_master_write_byte(cmd, LSB(WAKEUP), ACK_CHECK_EN);
  }
  i2c_master_stop(cmd);
  wakeup_err |= i2c_master_cmd_begin(i2c_num, cmd, pdMS_TO_TICKS(1000));
  i2c_cmd_link_delete(cmd);
  vTaskDelay(pdMS_TO_TICKS(10)); // wait 10 ms

  return wakeup_err;
}

static shtc3_err SHTC3_CheckCrc(uint8_t *data, uint8_t noOfBytes,
                                uint8_t checksum) {
  uint8_t bit;        // bit mask
  uint8_t crc = 0xFF; // calculated checksum
  uint8_t byteCtr;    // byte counter

  // calculates 8-Bit checksum with given polynomial
  for (byteCtr = 0; byteCtr < noOfBytes; byteCtr++) {
    crc ^= (data[byteCtr]);
    for (bit = 8; bit > 0; --bit) {
      if (crc & 0x80) {
        crc = (crc << 1) ^ CRC_POLYNOMIAL;
      } else {
        crc = (crc << 1);
      }
    }
  }

  // verify checksum
  if (crc != checksum) {
    return SENSOR_CRC_ERR;
  } else {
    return SENSOR_OK;
  }
}

static esp_err_t SHTC3_GetTempAndHumidity(i2c_port_t i2c_num, float *temp,
                                          float *humidity) {
  esp_err_t temp_humi_read_err;
  shtc3_err crc_err;
  uint8_t data_bytes[6];

  // if (temp_humi_read_err == ESP_OK)
  {
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    temp_humi_read_err = i2c_master_start(cmd);
    temp_humi_read_err = i2c_master_write_byte(
        cmd, SHTC3_SENSOR_ADDR << 1 | WRITE_BIT, ACK_CHECK_EN);
    temp_humi_read_err |=
        i2c_master_write_byte(cmd, MSB(T_RH_CLOCKSTR), ACK_CHECK_EN);
    temp_humi_read_err |=
        i2c_master_write_byte(cmd, LSB(T_RH_CLOCKSTR), ACK_CHECK_EN);

    i2c_master_stop(cmd);
    temp_humi_read_err |=
        i2c_master_cmd_begin(i2c_num, cmd, pdMS_TO_TICKS(1000));
    i2c_cmd_link_delete(cmd);

    cmd = i2c_cmd_link_create();
    temp_humi_read_err = i2c_master_start(cmd);
    temp_humi_read_err = i2c_master_write_byte(
        cmd, SHTC3_SENSOR_ADDR << 1 | READ_BIT, ACK_CHECK_EN);

    vTaskDelay(pdMS_TO_TICKS(100));

    if (temp_humi_read_err != ESP_OK)
      return temp_humi_read_err;

    for (int i = 0; i < 6; i++)
      temp_humi_read_err |= i2c_master_read_byte(cmd, &data_bytes[i], ACK_VAL);

    i2c_master_stop(cmd);
    temp_humi_read_err |=
        i2c_master_cmd_begin(i2c_num, cmd, pdMS_TO_TICKS(1000));
    i2c_cmd_link_delete(cmd);

    if (temp_humi_read_err == ESP_OK) {
      crc_err = SHTC3_CheckCrc(data_bytes, 2, data_bytes[2]);
      if (crc_err == SENSOR_OK) {
        if (temp != NULL)
          *temp = SHTC3_CalcTemperature(
              decode_2Byte_Parameter(data_bytes[0], data_bytes[1]));
      } else {
        ESP_LOGE(TAG, "CRC Error in read temperature data");
        temp_humi_read_err |= ESP_FAIL;
      }

      crc_err = SHTC3_CheckCrc(&data_bytes[3], 2, data_bytes[5]);
      if (crc_err == SENSOR_OK) {
        if (temp != NULL)
          *humidity = SHTC3_CalcHumidity(
              decode_2Byte_Parameter(data_bytes[3], data_bytes[4]));
      } else {
        ESP_LOGE(TAG, "CRC Error in read temperature data");
        temp_humi_read_err |= ESP_FAIL;
      }
    }
  }
  return temp_humi_read_err;
}

static esp_err_t SHTC3_GetId(i2c_port_t i2c_num, uint16_t *id) {
  esp_err_t id_read_err;
  shtc3_err crc_err;
  uint8_t bytes[2];
  uint8_t crc;

  // if (id_read_err == ESP_OK)
  {
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    id_read_err = i2c_master_start(cmd);
    id_read_err = i2c_master_write_byte(cmd, SHTC3_SENSOR_ADDR << 1 | WRITE_BIT,
                                        ACK_CHECK_EN);
    id_read_err |= i2c_master_write_byte(cmd, MSB(READ_ID), ACK_CHECK_EN);
    id_read_err |= i2c_master_write_byte(cmd, LSB(READ_ID), ACK_CHECK_EN);

    i2c_master_stop(cmd);
    id_read_err |= i2c_master_cmd_begin(i2c_num, cmd, pdMS_TO_TICKS(1000));
    i2c_cmd_link_delete(cmd);

    cmd = i2c_cmd_link_create();
    id_read_err = i2c_master_start(cmd);
    id_read_err = i2c_master_write_byte(cmd, SHTC3_SENSOR_ADDR << 1 | READ_BIT,
                                        ACK_CHECK_EN);

    vTaskDelay(pdMS_TO_TICKS(100));
    if (id_read_err != ESP_OK)
      return id_read_err;

    id_read_err |= i2c_master_read_byte(cmd, &bytes[0], ACK_VAL);
    id_read_err |= i2c_master_read_byte(cmd, &bytes[1], ACK_VAL);
    id_read_err |= i2c_master_read_byte(cmd, &crc, ACK_VAL);

    i2c_master_stop(cmd);
    id_read_err |= i2c_master_cmd_begin(i2c_num, cmd, pdMS_TO_TICKS(1000));
    i2c_cmd_link_delete(cmd);

    if (id_read_err == ESP_OK) {
      crc_err = SHTC3_CheckCrc(bytes, 2, crc);
      if (crc_err == SENSOR_OK) {
        if (id != NULL) {
          *id = decode_2Byte_Parameter(bytes[0], bytes[1]);
          printf("\nSensor id = %d\n", *id);
        }
      } else {
        ESP_LOGE(TAG, "CRC Error in read id data");
        id_read_err |= ESP_FAIL;
      }
    }
  }
  return id_read_err;
}

static esp_err_t SHTC3_SoftResetCommand(i2c_port_t i2c_num) {
  esp_err_t soft_reset_err;

  i2c_cmd_handle_t cmd = i2c_cmd_link_create();
  soft_reset_err = i2c_master_start(cmd);
  if (soft_reset_err == ESP_OK) {
    soft_reset_err = i2c_master_write_byte(
        cmd, SHTC3_SENSOR_ADDR << 1 | WRITE_BIT, ACK_CHECK_EN);
    soft_reset_err |= i2c_master_write_byte(cmd, MSB(SOFT_RESET), ACK_CHECK_EN);
    soft_reset_err |= i2c_master_write_byte(cmd, LSB(SOFT_RESET), ACK_CHECK_EN);
  }
  i2c_master_stop(cmd);
  soft_reset_err |= i2c_master_cmd_begin(i2c_num, cmd, pdMS_TO_TICKS(1000));
  i2c_cmd_link_delete(cmd);
  vTaskDelay(pdMS_TO_TICKS(10)); // wait 10 ms

  return soft_reset_err;
}

static esp_err_t SHTC3_SleepCommand(i2c_port_t i2c_num) {
  esp_err_t sleep_err;

  i2c_cmd_handle_t cmd = i2c_cmd_link_create();
  sleep_err = i2c_master_start(cmd);
  if (sleep_err == ESP_OK) {
    sleep_err |= i2c_master_write_byte(cmd, SHTC3_SENSOR_ADDR << 1 | WRITE_BIT,
                                       ACK_CHECK_EN);
    sleep_err |= i2c_master_write_byte(cmd, MSB(SLEEP), ACK_CHECK_EN);
    sleep_err |= i2c_master_write_byte(cmd, LSB(SLEEP), ACK_CHECK_EN);
  }
  i2c_master_stop(cmd);
  sleep_err |= i2c_master_cmd_begin(i2c_num, cmd, pdMS_TO_TICKS(1000));
  i2c_cmd_link_delete(cmd);
  vTaskDelay(pdMS_TO_TICKS(10)); // wait 10 ms

  return sleep_err;
}

static esp_err_t initiate_sensor_communication(uint16_t *sensor_id) {
  esp_err_t esp_err;

  if (SHTC3_WakeupCommand(I2C_MASTER_NUM) == ESP_OK) {
    vTaskDelay(pdMS_TO_TICKS(10));
    if (SHTC3_SoftResetCommand(I2C_MASTER_NUM) == ESP_OK) {
      vTaskDelay(pdMS_TO_TICKS(10));
      if (SHTC3_GetId(I2C_MASTER_NUM, sensor_id) == ESP_OK)
        return SENSOR_OK;
      else
        return SENSOR_READ_ID_FAIL;
    } else
      return SENSOR_SOFT_RST_FAIL;
  } else
    return SENSOR_WAKEUP_FAIL;
  return SENSOR_WAKEUP_FAIL;
}

static shtc3_err get_sensor_reading(i2c_port_t i2c_num, float *temp,
                                    float *humid) {
  esp_err_t ret;

  if (SHTC3_WakeupCommand(i2c_num) == ESP_OK) {
    if (SHTC3_GetTempAndHumidity(i2c_num, temp, humid) == ESP_OK) {
      ret = SHTC3_SleepCommand(i2c_num);
      if (ret != ESP_OK)
        ESP_LOGE(TAG, "Sensor Sleep failed, err : %s", esp_err_to_name(ret));
      return SENSOR_OK;
    } else
      return SENSOR_TEMP_HUMI_READ_FAIL;
  } else
    return SENSOR_WAKEUP_FAIL;
}

static void sensor_measurement_task(void *arg) {
  temp_humidity_sensor_t *sensor_settings = NULL;
  if (arg != NULL)
    sensor_settings = (temp_humidity_sensor_t *)arg;
  int i = 0;
  shtc3_err sensor_err;
  esp_err_t esp_err;
  void (*fun_ptr)(void) = NULL;
  uint32_t measurements_delay_time_ms = (60 * 1000);
  uint16_t sensor_id;
  uint8_t initial_timeout = 2;

  int cnt = 0;
  if (sensor_settings != NULL) {
    fun_ptr = sensor_settings->fun_ptr;
    measurements_delay_time_ms = sensor_settings->measurement_time_sec * 1000;
  }

  while (initial_timeout) {
    sensor_err = initiate_sensor_communication(&sensor_id);
    if (sensor_err == SENSOR_OK) {
      if (GET_BIT(sensor_id, 11) &&
          (sensor_id & SENSOR_PRODUCT_CODE_MASK) == SENSOR_PRODUCT_CODE) {
        ESP_LOGI(TAG, "sensor product code found correctly\n");
        break;
      } else {
        ESP_LOGE(TAG, "Wrong Sensor Product code found");
        vTaskDelay(pdMS_TO_TICKS(measurements_delay_time_ms));
        initial_timeout--;
      }
    } else {
      vTaskDelay(pdMS_TO_TICKS(measurements_delay_time_ms));
      initial_timeout--;
    }
  }

  if (!initial_timeout)
    goto delete_shtc3_task;

  while (1) {
    sensor_err = get_sensor_reading(I2C_MASTER_NUM, &Temperature, &Humidity);

    switch (sensor_err) {
    case SENSOR_OK:
      printf("sensor val humidity: %.02f %%\n", Humidity);
      printf("sensor val temprature: %.02f \n", Temperature);
      break;
    case SENSOR_WAKEUP_FAIL:
      ESP_LOGE(TAG, "Sensor failed to wakeup");
      break;
    case SENSOR_TEMP_HUMI_READ_FAIL:
      ESP_LOGE(TAG,
               "Sensor failed to measure and read temp and humidity reading");
      break;
    default:
      break;
    }

    if (fun_ptr != NULL) {
      fun_ptr();
    }

    vTaskDelay(pdMS_TO_TICKS(measurements_delay_time_ms));
  }
delete_shtc3_task:
  printf("deleting shtc3 sensor task\n");
  vSemaphoreDelete(print_mux);
  vTaskDelete(NULL);
}

int shtc3_Read(float *humidity, float *temperature) {
  *humidity = Humidity;
  *temperature = Temperature;
  return 0;
}

void app_i2c_shtc3(void (*fun_ptr)(void), uint32_t measurement_time_sec) {
  temp_humidity_sensor_t sensor_Settings = {
      .fun_ptr = fun_ptr, .measurement_time_sec = measurement_time_sec};
  print_mux = xSemaphoreCreateMutex();
  ESP_ERROR_CHECK(i2c_master_init(I2C_MASTER_FREQ_HZ));
  xTaskCreate(sensor_measurement_task, "sensor_measurement_task", 2024 * 2,
              (void *)&sensor_Settings, 10, NULL);
}

#if defined(CONFIG_UNITTEST_ENABLE_ALL) ||                                     \
    defined(CONFIG_AUTMA_DEVICE_TYPE_SIMULATOR)
esp_err_t unittest_shtc3(float *Temperature, float *Humidity) {

#ifdef CONFIG_AUTMA_DEVICE_TYPE_SIMULATOR
  static bool SIMULATOR_DEVICE_START = false;

  if (!SIMULATOR_DEVICE_START) {
    ESP_ERROR_CHECK(i2c_master_init(I2C_MASTER_FREQ_HZ));
    SIMULATOR_DEVICE_START = true;
  }
#elif CONFIG_UNITTEST_ENABLE_ALL
  ESP_ERROR_CHECK(i2c_master_init(I2C_MASTER_FREQ_HZ));
#endif

  shtc3_err sensor_err;
  uint16_t sensor_id;

  sensor_err = initiate_sensor_communication(&sensor_id);
  if (sensor_err == SENSOR_OK) {
    if (GET_BIT(sensor_id, 11) &&
        (sensor_id & SENSOR_PRODUCT_CODE_MASK) == SENSOR_PRODUCT_CODE) {
      ESP_LOGI(TAG, "sensor product code found correctly\n");
    } else {
      ESP_LOGE(TAG, "Wrong Sensor Product code found");
      ESP_LOGE("UNITTEST", "Temp & Humididty Sensor ISSUE");
      return ESP_FAIL;
    }
  } else {
    ESP_LOGE("UNITTEST", "Temp & Humididty Sensor ISSUE");
    return ESP_FAIL;
  }

  sensor_err = get_sensor_reading(I2C_MASTER_NUM, Temperature, Humidity);

  switch (sensor_err) {
  case SENSOR_OK:
    printf("sensor val humidity: %.02f %%\n", *Humidity);
    printf("sensor val temprature: %.02f \n", *Temperature);
    return ESP_OK;
  case SENSOR_WAKEUP_FAIL:
    ESP_LOGE(TAG, "Sensor failed to wakeup");
    break;
  case SENSOR_TEMP_HUMI_READ_FAIL:
    ESP_LOGE(TAG,
             "Sensor failed to measure and read temp and humidity reading");
    break;
  default:
    break;
  }
  ESP_LOGE("UNITTEST", "Temp & Humididty Sensor ISSUE");
  return ESP_FAIL;
}
#endif
