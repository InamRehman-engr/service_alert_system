#include "PMS5003.h"
#include "driver/uart.h"
#include "esp_log.h"
#include "string.h"
#include <stdio.h>
#define TAG "PMS5003_UART"

#define TXD_PIN 12
#define RXD_PIN 14

#define RX_BUF_SIZE 1024

QueueHandle_t uart_queue;
pms5003_measurement_t reading, averaged_reading;
pms5003_config_t pms0;
void PMS5003_event_task(pms5003_config_t *pms0) {
  uart_event_t uart_event;
  uint8_t *received_buffer = malloc(RX_BUF_SIZE);
  size_t datalen;
  while (true) {
    if (xQueueReceive(uart_queue, &uart_event, portMAX_DELAY)) {
      switch (uart_event.type) {
      case UART_DATA:
        ESP_LOGI(TAG, "UART_DATA");
        pms5003_make_measurement(pms0, &reading);
        break;
      case UART_BREAK:
        ESP_LOGI(TAG, "UART_BREAK");
        break;
      case UART_BUFFER_FULL:
        ESP_LOGI(TAG, "UART_BUFFER_FULL");
        break;
      case UART_FIFO_OVF:
        ESP_LOGI(TAG, "UART_FIFO_OVF");
        uart_flush_input(UART_NUM_1);
        xQueueReset(uart_queue);
        break;
      case UART_FRAME_ERR:
        ESP_LOGI(TAG, "UART_FRAME_ERR");
        break;
      case UART_PARITY_ERR:
        ESP_LOGI(TAG, "UART_PARITY_ERR");
        break;
      case UART_DATA_BREAK:
        ESP_LOGI(TAG, "UART_DATA_BREAK");
        break;
      case UART_PATTERN_DET:
        ESP_LOGI(TAG, "UART_PATTERN_DET");
        break;
      default:
        break;
      }
    }
    vTaskDelay(1 / portTICK_PERIOD_MS); // While loop delay...
  }
}

esp_err_t PMS5003_init(pms5003_config_t *inst) {
  // Initializing UART interface
  esp_err_t err;
  uart_config_t uart_config = {.baud_rate = 9600,
                               .data_bits = UART_DATA_8_BITS,
                               .parity = UART_PARITY_DISABLE,
                               .stop_bits = UART_STOP_BITS_1,
                               .flow_ctrl = UART_HW_FLOWCTRL_DISABLE};

  uart_param_config(inst->uart_instance, &uart_config);
  err = uart_set_pin(inst->uart_instance, inst->txd_pin, inst->rxd_pin,
                     UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
  if (err != ESP_OK) {
    ESP_LOGE(TAG, "UART set pins failed");
    return err;
  }

  err = uart_driver_install(inst->uart_instance, inst->uart_buffer_size * 2, 0,
                            20, &uart_queue, 0);
  if (err != ESP_OK) {
    ESP_LOGE(TAG, "UART driver install failed");
    return err;
  }
  // Initializing GPIO pins                                   // Set and reset
  // pin of PMS5003 are internally pulled up and should not be used unless
  // needed
  gpio_config_t io_conf;
  io_conf.intr_type = GPIO_INTR_DISABLE;
  io_conf.mode = GPIO_MODE_OUTPUT;
  io_conf.pin_bit_mask = (1ULL << inst->set_pin) | (1ULL << inst->reset_pin) |
                         (1ULL << inst->mode_pin);
  io_conf.pull_down_en = 0;
  io_conf.pull_up_en = 0;
  gpio_config(&io_conf);

  gpio_set_level(inst->set_pin, 0);
  gpio_set_level(inst->reset_pin, 1);
  gpio_set_level(inst->mode_pin, 1);
  return ESP_OK;
}

void pms5003_make_measurement(pms5003_config_t *inst,
                              pms5003_measurement_t *reading) {
  uint8_t *data = (uint8_t *)malloc(inst->uart_buffer_size);

  // gpio_set_level(inst->set_pin, 1);                            // Set and
  // reset pin of PMS5003 are internally pulled up and should not be used unless
  // needed
  vTaskDelay(1000 / portTICK_PERIOD_MS); // Wait for a second...
  uart_flush(UART_NUM_1);

  pms5003_measurement_t current_reading;

  reading->pm1_0_std = 0; // Clearing Measurement
  reading->pm2_5_std = 0;
  reading->pm10_std = 0;
  reading->pm1_0_atm = 0;
  reading->pm2_5_atm = 0;
  reading->pm10_atm = 0;

  int measurements_number = 0;
  while (measurements_number < 10) {
    int len = uart_read_bytes(UART_NUM_1, data, inst->uart_buffer_size, 1);
    if (data == NULL)
      continue;
    ESP_LOG_BUFFER_HEXDUMP("UART_REC", data, len, ESP_LOG_DEBUG);
    if (len > 0 && pms5003_process_data(len, data, &current_reading)) {
      reading->pm1_0_std += current_reading.pm1_0_std;
      reading->pm2_5_std += current_reading.pm2_5_std;
      reading->pm10_std += current_reading.pm10_std;
      reading->pm1_0_atm += current_reading.pm1_0_atm;
      reading->pm2_5_atm += current_reading.pm2_5_atm;
      reading->pm10_atm += current_reading.pm10_atm;
      measurements_number++;
    }
    vTaskDelay(1 / portTICK_PERIOD_MS); // While loop delay...
  }
  reading->pm1_0_std /= 10.0; // Avereging data
  reading->pm2_5_std /= 10.0;
  reading->pm10_std /= 10.0;
  reading->pm1_0_atm /= 10.0;
  reading->pm2_5_atm /= 10.0;
  reading->pm10_atm /= 10.0;
  averaged_reading = *reading;
  // gpio_set_level(inst->set_pin, 0);                               // Set and
  // reset pin of PMS5003 are internally pulled up and should not be used unless
  // needed
}

int pms5003_process_data(int len, uint8_t *data,
                         pms5003_measurement_t *reading) {

  if (len != 32) // Length test: The length should be 32
  {
    return 0;
  }

  if (data[0] != 0x42 || data[1] != 0x4D || data[2] != 0x00 ||
      data[3] != 0x1C) // Start of frame delimiter test
  {
    return 0;
  }

  int checksum = 0, checksum_h, checksum_l; // Checksum test
  for (int i = 0; i < 30; i++)
    checksum += data[i];
  checksum_h = (checksum >> 8) & 0xFF;
  checksum_l = checksum & 0xFF;
  if (data[30] != checksum_h || data[31] != checksum_l)
    return 0;
  reading->pm1_0_std = (data[4] << 8) + data[5]; // Reading data
  reading->pm2_5_std = (data[6] << 8) + data[7];
  reading->pm10_std = (data[8] << 8) + data[9];
  reading->pm1_0_atm = (data[10] << 8) + data[11];
  reading->pm2_5_atm = (data[12] << 8) + data[13];
  reading->pm10_atm = (data[14] << 8) + data[15];

  return 1;
}

void pms5003_read(pms5003_measurement_t *Reading) {
  *Reading = averaged_reading;
}

esp_err_t PMS5003_task(pms5003_config_t *x) {
  esp_err_t err;
  err = xTaskCreate(PMS5003_event_task, "uart_event_task", 2048, x, 10, NULL);
  if (err != ESP_OK)
    return err;
  return ESP_OK;
}
esp_err_t PMS5003_sleep(pms5003_config_t *inst) {
  esp_err_t err;
  err = gpio_set_level(inst->set_pin, 1);
  if (err != ESP_OK) {
    ESP_LOGE(TAG, "Error in PMS5003 sleep function");
    return err;
  }
  vTaskDelay(500 / portTICK_PERIOD_MS);
  ESP_LOGD(TAG, "PMS5003 sleep");
  return ESP_OK;
}
esp_err_t PMS5003_wakeUp(pms5003_config_t *inst) {
  esp_err_t err;
  err = gpio_set_level(inst->set_pin, 1);
  if (err != ESP_OK) {
    return err;
    ESP_LOGE(TAG, "Error in PMS5003 WakeUp function");
  }
  vTaskDelay(500 / portTICK_PERIOD_MS);
  ESP_LOGD(TAG, "PMS5003 WakeUp");
  return ESP_OK;
}
esp_err_t PMS5003_reset(pms5003_config_t *inst) {
  gpio_set_level(inst->reset_pin, 0);
  vTaskDelay(10000 / portTICK_PERIOD_MS);
  gpio_set_level(inst->reset_pin, 1);
  ESP_LOGD(TAG, "PMS5003 reset");
  return ESP_OK;
}