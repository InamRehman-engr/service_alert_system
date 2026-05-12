
#include "AHT10.h"
#include "driver/i2c.h"
#include "esp_err.h"
#include "esp_log.h"
#include "esp_types.h"
#include "th_i2c.h"
#define _I2C_NUMBER(num) I2C_NUM_##num
#define I2C_NUMBER(num) _I2C_NUMBER(num)

#define I2C_MASTER_SCL_IO 25     /*!< gpio number for I2C master clock */
#define I2C_MASTER_SDA_IO 26     /*!< gpio number for I2C master data  */
#define I2C_MASTER_NUM I2C_NUM_0 /*!< I2C port number for master dev */
const uint32_t I2C_MASTER_FREQ_HZ = 100000; /*!< I2C master clock frequency */
#define I2C_MASTER_TX_BUF_DISABLE 0 /*!< I2C master doesn't need buffer */
#define I2C_MASTER_RX_BUF_DISABLE 0 /*!< I2C master doesn't need buffer */
SemaphoreHandle_t print_mux = NULL;
static float Humidity = 0;
static float Temperature = 0;

typedef enum {
  INIT_ID = 0xE1,     // Initialization/Calibration
  SOFT_RESET = 0xBA,  // soft reset
  TRIGGER_CMD = 0xAC, // Trigger Reading
} aht10_Commands;

typedef enum {
  // SENSOR_FAIL=0X00,
  SENSOR_OK = 0X01,
  SENSOR_WAKEUP_FAIL = 0x02,
  SENSOR_TEMP_READ_FAIL = 0x03,
  SENSOR_HUMI_READ_FAIL = 0x04,
  SENSOR_READ_ID_FAIL = 0x05,
  SENSOR_SOFT_RST_FAIL = 0x06,
  SENSOR_CRC_ERR = 0x07,
} aht10_err;

esp_err_t i2c_init(void) {

  i2c_config_t i2c_config = {.mode = I2C_MODE_MASTER,
                             .sda_io_num = I2C_MASTER_SDA_IO,
                             .scl_io_num = I2C_MASTER_SCL_IO,
                             .sda_pullup_en = GPIO_PULLUP_DISABLE,
                             .scl_pullup_en = GPIO_PULLUP_DISABLE,
                             .master.clk_speed = I2C_MASTER_FREQ_HZ};
  i2c_param_config(I2C_MASTER_NUM, &i2c_config);
  return i2c_driver_install(I2C_MASTER_NUM, i2c_config.mode, 0, 0, 0);
}

esp_err_t readHumidity(const i2c_port_t i2c_num, float *humidity) {
  esp_err_t ret = getHumidityReading(i2c_num, AHT10_TRIGGER_MEASUREMENT,
                                     humidity, &ConvertRawHumidityToPercentage);
  if (ret == ESP_OK) {
    return SENSOR_OK;
  } else {
    return SENSOR_HUMI_READ_FAIL;
  }
}

esp_err_t readTemperature(const i2c_port_t i2c_num, float *temperature) {
  esp_err_t ret =
      getTemperatureReading(i2c_num, AHT10_TRIGGER_MEASUREMENT, temperature,
                            &ConvertRawTemperatureToCelcius);
  if (ret == ESP_OK) {
    return SENSOR_OK;
  } else {
    return SENSOR_TEMP_READ_FAIL;
  }
}
esp_err_t getTemperatureReading(const i2c_port_t i2c_num,
                                const uint8_t *command, float *output,
                                float (*fn)(const uint32_t)) {
  esp_err_t ret = writeCommandBytes(i2c_num, command, 3);

  if (ret != ESP_OK) {
    ESP_LOGE("TEMPERATURE_READ", "I2C write failed with code:%d", ret);
    return ret;
  }
  // delay for 100ms between write and read calls, as the sensor can take
  // some time to respond.
  vTaskDelay(100 / portTICK_PERIOD_MS);

  uint8_t buf[6];
  uint32_t temp_buf1;
  uint32_t temp_buf2;
  uint32_t temp_buf;
  ret = readResponseBytes(i2c_num, buf, 6);

  if (ret != ESP_OK) {
    ESP_LOGE("TEMPERATURE_READ", "I2C read failed with code:%d", ret);
    return ret;
  }
  // re-assemble the bytes, and call the specified code-conversion function
  // Visit data sheet on how the bytes should be assembled
  // https://server4.eca.ir/eshop/AHT10/Aosong_AHT10_en_draft_0c.pdf
  // to finally retrieve our final sensor reading value, writing it out to
  // the output pointer.

  temp_buf1 = (buf[3] & 0x0f) << 16;
  temp_buf2 = buf[4] << 8;
  temp_buf = temp_buf1 | temp_buf2 | buf[5];
  *output = fn(temp_buf);

  return ESP_OK;
}

esp_err_t getHumidityReading(const i2c_port_t i2c_num, const uint8_t *command,
                             float *output, float (*fn)(const uint32_t)) {
  esp_err_t ret = writeCommandBytes(i2c_num, command, 3);

  if (ret != ESP_OK) {
    ESP_LOGE("HUMIDITY_READ", "I2C write failed with code:%d", ret);
    return ret;
  }
  // delay for 100ms between write and read calls, as the sensor can take
  // some time to respond.
  vTaskDelay(100 / portTICK_PERIOD_MS);

  uint8_t buf[6];
  uint32_t humidity_buf1;
  uint32_t humidity_buf2;
  uint32_t humidity_buf3;
  uint32_t humidity_buf;
  ret = readResponseBytes(i2c_num, buf, 6);

  if (ret != ESP_OK) {
    ESP_LOGE("HUMIDITY_READ", "I2C read failed with code:%d", ret);
    return ret;
  }
  // re-assemble the bytes, and call the specified code-conversion function
  // Visit data sheet on how the bytes should be assembled
  // https://server4.eca.ir/eshop/AHT10/Aosong_AHT10_en_draft_0c.pdf
  // to finally retrieve our final sensor reading value, writing it out to
  // the output pointer.

  humidity_buf1 = buf[1] << 12;
  humidity_buf2 = buf[2] << 4;
  humidity_buf3 = buf[3] >> 4;
  humidity_buf = humidity_buf1 | humidity_buf2 | humidity_buf3;
  *output = fn(humidity_buf);
  return ESP_OK;
}

// hardware interaction

esp_err_t readResponseBytes(const i2c_port_t i2c_num, uint8_t *output,
                            const size_t nbytes) {
  i2c_cmd_handle_t cmd = i2c_cmd_link_create();
  i2c_master_start(cmd);

  // write the 7-bit address of the sensor to the queue, using the last bit
  // to indicate we are performing a read.
  i2c_master_write_byte(cmd, AHT10_I2C_ADDR << 1 | I2C_MASTER_READ,
                        ACK_CHECK_EN);

  // read nbytes number of bytes from the response into the buffer. make
  // sure we send a NACK with the final byte rather than an ACK.
  for (size_t i = 0; i < nbytes; i++) {
    i2c_master_read_byte(cmd, &output[i], i == nbytes - 1 ? NACK_VAL : ACK_VAL);
  }

  // send all queued commands, blocking until all commands have been sent.
  // note that this is *not* thread-safe.
  i2c_master_stop(cmd);
  esp_err_t ret =
      i2c_master_cmd_begin(i2c_num, cmd, I2C_TIMEOUT_MS / portTICK_PERIOD_MS);
  i2c_cmd_link_delete(cmd);

  return ret;
}

esp_err_t writeCommandBytes(const i2c_port_t i2c_num,
                            const uint8_t *i2c_command, const size_t nbytes) {
  i2c_cmd_handle_t cmd = i2c_cmd_link_create();
  i2c_master_start(cmd);

  // write the 7-bit address of the sensor to the bus, using the last bit to
  // indicate we are performing a write.
  i2c_master_write_byte(cmd, AHT10_I2C_ADDR << 1 | I2C_MASTER_WRITE,
                        ACK_CHECK_EN);

  for (size_t i = 0; i < nbytes; i++) {
    i2c_master_write_byte(cmd, i2c_command[i], 1);
  }
  // send all queued commands, blocking until all commands have been sent.
  // note that this is *not* thread-safe.
  i2c_master_stop(cmd);
  esp_err_t ret =
      i2c_master_cmd_begin(i2c_num, cmd, I2C_TIMEOUT_MS / portTICK_PERIOD_MS);
  i2c_cmd_link_delete(cmd);

  return ret;
}

// conversion functions

float ConvertRawHumidityToPercentage(const uint32_t rh_code) {
  // refer to datasheet for more information.

  return (((rh_code) / 1048576.0) * 100.0);
}

float ConvertRawTemperatureToCelcius(const uint32_t temp_code) {
  // refer to datasheet for more information.
  return (((temp_code) / 1048576.0) * 200.0) - 50.0;
}

static void sensor_measurement_task(void *arg) {
  temp_humidity_sensor_t *sensor_settings = NULL;
  if (arg != NULL)
    sensor_settings = (temp_humidity_sensor_t *)arg;

  aht10_err sensor_err_humidity;
  aht10_err sensor_err_tempreture;
  while (1) {

    sensor_err_humidity = readHumidity(I2C_NUM_0, &Humidity);
    sensor_err_tempreture = readTemperature(I2C_NUM_0, &Temperature);

    switch (sensor_err_humidity) {
    case SENSOR_OK:
      ESP_LOGD("SENSOR_OUT_PUT", "sensor val humidity: %.02f %%\n", Humidity);
      break;
    case SENSOR_HUMI_READ_FAIL:
      ESP_LOGE("AHT10", "Sensor failed to measure and read humidity reading");
      break;
    default:
      break;
    }

    switch (sensor_err_tempreture) {
    case SENSOR_OK:
      ESP_LOGD("SENSOR_OUT_PUT", "sensor val tempreture: %.02f %%\n",
               Temperature);
      break;
    case SENSOR_TEMP_READ_FAIL:
      ESP_LOGE("AHT10", "Sensor failed to measure and read Tempreture reading");
      break;
    default:
      break;
    }
  }
}

int aht10_Read(float *humidity, float *temperature) {
  *humidity = Humidity;
  *temperature = Temperature;
  return 0;
}

void app_i2c_aht10(void (*fun_ptr)(void), uint32_t measurement_time_sec) {
  temp_humidity_sensor_t sensor_Settings = {
      .fun_ptr = fun_ptr, .measurement_time_sec = measurement_time_sec};
  print_mux = xSemaphoreCreateMutex();
  ESP_ERROR_CHECK(i2c_init());
  xTaskCreate(sensor_measurement_task, "sensor_measurement_task", 2024 * 2,
              (void *)&sensor_Settings, 10, NULL);
}
