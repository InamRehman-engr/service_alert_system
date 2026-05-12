#include "uart_driver.h"
#include "esp_log.h"

#define PATTERN_CHR_NUM (3)
#define TAG "UART"
const int MAX_SEND_BUFFER_LENGTH = 2048;
const int MAX_RECEIVE_BUFFER_LENGTH = 2048;

uart_error_codes uart_receive_data(QueueHandle_t *uart_queue,
                                   uart_port_t uart_port_no, uint8_t *data,
                                   int receive_buffer_size,
                                   TickType_t uart_receive_timeout_ticks,
                                   int *number_of_bytes_received) {
  if (data == NULL) {
    ESP_LOGE(TAG, "Invalid data buffer");
    return INVALID_DATA_BUFFER_POINTER;
  }
  if (uart_queue == NULL) {
    ESP_LOGE(TAG, "Invalid queue handle");
    return UART_FAIL;
  }

  uart_event_t event;
  size_t buffered_size;
  int rx_data_length = 0;
  int receieved_len = 0;
  bool n_check =
      xQueueReceive(*uart_queue, (void *)&event, uart_receive_timeout_ticks);
  // Waiting for UART event.
  if (n_check) {
    memset(data, 0, receive_buffer_size);
    ESP_LOGW(TAG, "inside queue with event : %d", event.type);
    //   memset(recieved_serial_data,0, RD_BUF_SIZE);
    //    ESP_LOGI(TAG, "uart[%d] event:", EX_UART_NUM);
    switch (event.type) {
      // Event of UART receving data
      /*We'd better handler data event fast, there would be much more data
      events than other types of events. If we take too much time on data event,
      the queue might be full.*/

    case UART_DATA:
      uart_get_buffered_data_len(uart_port_no, (size_t *)&rx_data_length);
      if (receive_buffer_size < rx_data_length) {
        ESP_LOGE(TAG, "Receive buffer is too small");
        uart_flush(uart_port_no);
        return INVALID_UART_RECEIVE_BUFFER_SIZE;
      }

      receieved_len =
          uart_read_bytes(uart_port_no, data, event.size, portMAX_DELAY);
      vTaskDelay(10 / portTICK_PERIOD_MS);
      ESP_LOGW(TAG, "length : %d", receieved_len);

      *number_of_bytes_received = receieved_len;
      uart_flush(uart_port_no);
      return UART_DATA_RECEIVE_SUCCESS;
      break;
    // Event of HW FIFO overflow detected
    case UART_FIFO_OVF:
      ESP_LOGW(TAG, "hw fifo overflow");
      // If fifo overflow happened, you should consider adding flow control for
      // your application. The ISR has already reset the rx FIFO, As an example,
      // we directly flush the rx buffer here in order to read more data.
      uart_flush_input(uart_port_no);
      xQueueReset(*uart_queue);
      break;
    // Event of UART ring buffer full
    case UART_BUFFER_FULL:
      ESP_LOGW(TAG, "ring buffer full");
      // If buffer full happened, you should consider encreasing your buffer
      // size As an example, we directly flush the rx buffer here in order to
      // read more data.
      uart_flush_input(uart_port_no);
      xQueueReset(*uart_queue);
      break;
    // Event of UART RX break detected
    case UART_BREAK:
      ESP_LOGW(TAG, "uart rx break");
      break;
    // Event of UART parity check error
    case UART_PARITY_ERR:
      ESP_LOGE(TAG, "uart parity error");
      break;
    // Event of UART frame error
    case UART_FRAME_ERR:
      ESP_LOGE(TAG, "uart frame error");
      break;
    // UART_PATTERN_DET
    case UART_PATTERN_DET:
      uart_get_buffered_data_len(uart_port_no, &buffered_size);
      int pos = uart_pattern_pop_pos(uart_port_no);
      ESP_LOGI(TAG, "[UART PATTERN DETECTED] pos: %d, buffered size: %zu", pos,
               buffered_size);
      if (pos == -1) {
        // There used to be a UART_PATTERN_DET event, but the pattern position
        // queue is full so that it can not record the position. We should set a
        // larger queue size. As an example, we directly flush the rx buffer
        // here.
        uart_flush_input(uart_port_no);
      } else {
        uart_read_bytes(uart_port_no, data, pos, 100 / portTICK_PERIOD_MS);
        uint8_t pat[PATTERN_CHR_NUM + 1];
        memset(pat, 0, sizeof(pat));
        uart_read_bytes(uart_port_no, pat, PATTERN_CHR_NUM,
                        100 / portTICK_PERIOD_MS);
        ESP_LOGI(TAG, "read data: %s", data);
        ESP_LOGI(TAG, "read pat : %s", pat);
      }
      break;
    // Others
    default:
      ESP_LOGI(TAG, "uart event type: %d", event.type);
      break;
    }
  }
  return UART_DATA_RECEIVE_ERROR;
}

uart_error_codes uart_send_data(uart_port_t uart_port_no, uint8_t *data,
                                size_t data_len, int *number_of_bytes_written) {
  // Pointer validity check
  if (data == NULL) {
    ESP_LOGE(TAG, "Invalid pointer to data buffer");
    return INVALID_DATA_BUFFER_POINTER;
  }

  if (number_of_bytes_written == NULL) {
    ESP_LOGE(TAG, "Invalid pointer to number_of_bytes_written");
    return INVALID_POINTER;
  }

  // data length check
  if (data_len == 0 || data_len > MAX_SEND_BUFFER_LENGTH) {
    ESP_LOGE(TAG,
             "Invalid data buffer length! Data length must be greater than 0 "
             "and less than %d",
             MAX_SEND_BUFFER_LENGTH);
    return INVALID_UART_SEND_BUFFER_SIZE;
  }

  // send data
  int ret = uart_write_bytes(uart_port_no, data, data_len);
  if (ret == -1) {
    ESP_LOGE(TAG, "Failed to send data: %d", ret);
    return UART_DATA_SEND_ERROR;
  }
  ESP_LOGI(TAG, "Data sent successfully: %d bytes", ret);
  *number_of_bytes_written = ret;
  return UART_DATA_SEND_SUCCESS;
}

esp_err_t uart_send_and_receive_data(QueueHandle_t *uart_queue,
                                     uart_port_t uart_port_no,
                                     uint8_t *data_receive_buff,
                                     int receive_buffer_size,
                                     uint8_t *data_send_buff, int send_data_len,
                                     TickType_t uart_receive_timeout_ticks) {
  int number_of_bytes_sent = 0;
  int number_of_bytes_received = 0;

  uart_error_codes ret = uart_send_data(uart_port_no, data_send_buff,
                                        send_data_len, &number_of_bytes_sent);
  if (ret != UART_DATA_SEND_SUCCESS) {
    return ESP_FAIL;
  }

  ret = uart_receive_data(uart_queue, uart_port_no, data_receive_buff,
                          receive_buffer_size, uart_receive_timeout_ticks,
                          &number_of_bytes_received);
  if (ret != UART_DATA_RECEIVE_SUCCESS) {
    return ESP_FAIL;
  }
  return ESP_OK;
}

esp_err_t uart_send_single_byte(uart_port_t uart_port_no, uint8_t *data) {
  int number_of_bytes_written = 0;
  uart_error_codes ret =
      uart_send_data(uart_port_no, data, 1, &number_of_bytes_written);
  if (ret != UART_DATA_SEND_SUCCESS) {
    return ESP_FAIL;
  }
  return ESP_OK;
}

esp_err_t uart_receive_single_byte(QueueHandle_t *uart_queue,
                                   uart_port_t uart_port_no, uint8_t *data,
                                   TickType_t uart_receive_timeout_ticks) {
  int number_of_bytes_receive = 0;
  uart_error_codes ret =
      uart_receive_data(uart_queue, uart_port_no, data, 1,
                        uart_receive_timeout_ticks, &number_of_bytes_receive);
  if (ret != UART_DATA_RECEIVE_SUCCESS) {
    return ESP_FAIL;
  }
  return ESP_OK;
}

void uart_deinit(uart_port_t uart_port) {
  // Check is driver is installed or not.
  if (uart_is_driver_installed(uart_port)) {
    ESP_ERROR_CHECK(uart_driver_delete(uart_port));
    ESP_LOGI(TAG, "Deleted the UART driver associated with UART port: %d",
             uart_port);
  } else {
    ESP_LOGI(TAG, "UART driver associated with UART port: %d is NOT installed!",
             uart_port);
  }
}

/// TODO: Change driver to work with multiple queues from different. Implement
/// this by moving the queue object inside

bool uart_init(uart_peripheral *uart_inst, uart_port_t uart_port_num,
               uart_config_t *uart_config, uart_pin_config_t pin_config,
               uart_mode_t mode, int event_queue_size, int tx_buffer_size,
               int rx_buffer_size) {
  if (uart_inst == NULL || uart_inst->uart_queue == NULL ||
      uart_config == NULL) {
    ESP_LOGE(TAG,
             "Please provide complete parameters UART driver not initialize");
    return false;
  }

  uart_inst->uart_receive_data = uart_receive_data;
  uart_inst->uart_send_data = uart_send_data;
  uart_inst->uart_receive_single_byte = uart_receive_single_byte;
  uart_inst->uart_send_single_byte = uart_send_single_byte;
  uart_inst->uart_send_and_receive_data = uart_send_and_receive_data;
  uart_inst->uart_deinit = uart_deinit;
  int intr_alloc_flags = 0; // Disable interrupts
  ESP_ERROR_CHECK(uart_driver_install(
      uart_port_num, rx_buffer_size, tx_buffer_size, event_queue_size,
      &uart_inst->uart_queue, intr_alloc_flags));
  ESP_ERROR_CHECK(uart_param_config(uart_port_num, uart_config));
  ESP_ERROR_CHECK(uart_set_pin(uart_port_num, pin_config.TXD, pin_config.RXD,
                               pin_config.RTS, pin_config.CTS));
  ESP_ERROR_CHECK(uart_set_mode(uart_port_num, mode));
  ESP_LOGI(TAG, "UART driver for port: %d is initialized!", uart_port_num);
  return true;
}
