#include "si7006-a20.h"

#include "driver/i2c.h"
#include "esp_log.h"
#include "sdkconfig.h"
#include "th_i2c.h"
#include <stdio.h>

static const char *TAG = "i2c-example";

#define _I2C_NUMBER(num) I2C_NUM_##num
#define I2C_NUMBER(num) _I2C_NUMBER(num)

#define I2C_MASTER_FREQ_HZ 100000 /*!< I2C master clock frequency */

#define DELAY_TIME_BETWEEN_ITEMS_MS                                            \
  60 * 1000 /*!< delay time between different test items */

#define CMD_MEASURE_HUMIDITY_HOLD 0xE5
#define CMD_MEASURE_HUMIDITY_NO_HOLD 0xF5
#define CMD_MEASURE_TEMPERATURE_HOLD 0xE3
#define CMD_MEASURE_TEMPERATURE_NO_HOLD 0xF3
#define CMD_MEASURE_THERMISTOR_HOLD 0xEE
#define CMD_READ_PREVIOUS_TEMPERATURE 0xE0
#define CMD_RESET 0xFE
#define CMD_WRITE_REGISTER_1 0xE6
#define CMD_READ_REGISTER_1 0xE7
#define CMD_WRITE_REGISTER_2 0x50
#define CMD_READ_REGISTER_2 0x10
#define CMD_WRITE_REGISTER_3 0x51
#define CMD_READ_REGISTER_3 0x11
#define CMD_WRITE_COEFFICIENT 0xC5
#define CMD_READ_COEFFICIENT 0x84

#define BH1750_SENSOR_ADDR 0x40 /*!< slave address for BH1750 sensor */
#define BH1750_CMD_START CMD_MEASURE_HUMIDITY_NO_HOLD /*!< Operation mode */
#define WRITE_BIT 0                                   /*!< I2C master write */
#define READ_BIT 1                                    /*!< I2C master read */
#define ACK_CHECK_EN 0x1  /*!< I2C master will check ack from slave*/
#define ACK_CHECK_DIS 0x0 /*!< I2C master will not check ack from slave */
#define ACK_VAL 0x0       /*!< I2C ack value */
#define NACK_VAL 0x1      /*!< I2C nack value */

static float Humidity = 0;
static float Temperature = 0;
SemaphoreHandle_t print_mux = NULL;

/**
 * @brief test function to show buffer
 */
static void disp_buf(uint8_t *buf, int len) {
  int i;
  for (i = 0; i < len; i++) {
    printf("%02x ", buf[i]);
    if ((i + 1) % 16 == 0) {
      printf("\n");
    }
  }
  printf("\n");
}

/**
 * @brief test code to operate on BH1750 sensor
 *
 * 1. set operation mode(e.g One time L-resolution mode)
 * _________________________________________________________________
 * | start | slave_addr + wr_bit + ack | write 1 byte + ack  | stop |
 * --------|---------------------------|---------------------|------|
 * 2. wait more than 24 ms
 * 3. read data
 * ______________________________________________________________________________________
 * | start | slave_addr + rd_bit + ack | read 1 byte + ack  | read 1 byte + nack
 * | stop |
 * --------|---------------------------|--------------------|--------------------|------|
 */
static esp_err_t i2c_master_sensor_test(i2c_port_t i2c_num, uint8_t reg,
                                        uint8_t *data_h, uint8_t *data_l) {
  int ret;
  i2c_cmd_handle_t cmd = i2c_cmd_link_create();
  i2c_master_start(cmd);
  i2c_master_write_byte(cmd, BH1750_SENSOR_ADDR << 1 | WRITE_BIT, ACK_CHECK_EN);
  i2c_master_write_byte(cmd, reg, ACK_CHECK_EN);
  i2c_master_stop(cmd);
  ret = i2c_master_cmd_begin(i2c_num, cmd, pdMS_TO_TICKS(1000));
  i2c_cmd_link_delete(cmd);
  if (ret != ESP_OK) {
    return ret;
  }
  vTaskDelay(pdMS_TO_TICKS(1000));
  cmd = i2c_cmd_link_create();
  i2c_master_start(cmd);
  i2c_master_write_byte(cmd, BH1750_SENSOR_ADDR << 1 | READ_BIT, ACK_CHECK_EN);
  i2c_master_read_byte(cmd, data_h, ACK_VAL);
  i2c_master_read_byte(cmd, data_l, NACK_VAL);
  i2c_master_stop(cmd);
  ret = i2c_master_cmd_begin(i2c_num, cmd, pdMS_TO_TICKS(1000));
  i2c_cmd_link_delete(cmd);
  return ret;
}

/**
 * @brief i2c master initialization
 */

static void i2c_test_task(void *arg) {
  temp_humidity_sensor_t *sensor_settings = (temp_humidity_sensor_t *)arg;
  int i = 0;
  int ret;
  void (*fun_ptr)(void) = NULL;
  uint32_t measurements_delay_time_ms = (60 * 1000);
  // uint8_t *data = (uint8_t *)malloc(DATA_LENGTH);
  // uint8_t *data_wr = (uint8_t *)malloc(DATA_LENGTH);
  // uint8_t *data_rd = (uint8_t *)malloc(DATA_LENGTH);
  uint8_t sensor_data_h, sensor_data_l;
  int cnt = 0;
  if (arg != NULL) {
    fun_ptr = sensor_settings->fun_ptr;
    measurements_delay_time_ms = sensor_settings->measurement_time_sec * 1000;
  }

  while (1) {
    ESP_LOGI(TAG, "TASK[] test cnt: %d", cnt++);
    ret = i2c_master_sensor_test(I2C_MASTER_NUM, CMD_MEASURE_HUMIDITY_NO_HOLD,
                                 &sensor_data_h, &sensor_data_l);
    if (ret == ESP_ERR_TIMEOUT) {
      ESP_LOGE(TAG, "I2C Timeout");
    } else if (ret == ESP_OK) {
      Humidity =
          ((125 * (float)(sensor_data_h << 8 | sensor_data_l) / 65536) - 6);
      printf("sensor val: %.02f %%\n", Humidity);
    } else {
      ESP_LOGW(TAG, "%s: No ack, sensor not connected...skip...",
               esp_err_to_name(ret));
    }

    ret = i2c_master_sensor_test(I2C_MASTER_NUM, CMD_READ_PREVIOUS_TEMPERATURE,
                                 &sensor_data_h, &sensor_data_l);
    if (ret == ESP_ERR_TIMEOUT) {
      ESP_LOGE(TAG, "I2C Timeout");
    } else if (ret == ESP_OK) {
      Temperature =
          ((175.72 * (float)(sensor_data_h << 8 | sensor_data_l) / 65536) -
           46.85);
      printf("sensor val: %.02f C\n", Temperature);
    } else {
      ESP_LOGW(TAG, "%s: No ack, sensor not connected...skip...",
               esp_err_to_name(ret));
    }
    if (fun_ptr != NULL) {
      fun_ptr();
    }

    vTaskDelay(pdMS_TO_TICKS(measurements_delay_time_ms));
  }
  vSemaphoreDelete(print_mux);
  vTaskDelete(NULL);
}

int Si7006_Read(float *humidity, float *temperature) {
  *humidity = Humidity;
  *temperature = Temperature;
  return 0;
}

void app_i2c_si7006(void (*fun_ptr)(void), uint32_t measurement_time_sec) {
  temp_humidity_sensor_t sensor_Settings = {
      .fun_ptr = fun_ptr, .measurement_time_sec = measurement_time_sec};
  print_mux = xSemaphoreCreateMutex();
  ESP_ERROR_CHECK(i2c_master_init(I2C_MASTER_FREQ_HZ));
  xTaskCreate(i2c_test_task, "i2c_test_task_0", 2024 * 2,
              (void *)sensor_Settings, 10, NULL);
}