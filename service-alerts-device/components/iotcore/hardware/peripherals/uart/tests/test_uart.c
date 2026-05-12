#include "test_uart_common.h"
#include "unity.h"

TEST_CASE("Check function pointer initialization", "[uart][init]") {
  // Function call
  uart_init(&uart_func, uart_port, &uart_config, pinconfig, send_buffer,
            recv_buffer);

  // Assertions
  TEST_ASSERT_NOT_NULL(uart_func.uart_receive_data);
  TEST_ASSERT_NOT_NULL(uart_func.uart_send_data);
  TEST_ASSERT_NOT_NULL(uart_func.uart_receive_single_byte);
  TEST_ASSERT_NOT_NULL(uart_func.uart_send_single_byte);
  TEST_ASSERT_NOT_NULL(uart_func.uart_send_and_receive_data);
  TEST_ASSERT_NOT_NULL(uart_func.uart_deinit);
}

//====================================================================================================================================

/**
 * @note Baud rate value accessed via the uart_get_baudrate() function will
 * differ from the actual configured value The difference between actual and
 * obtained baud rates will be higher for larger values of baudrate.
 */
TEST_CASE("Check initialized baud rate", "[uart][init]") {
  uint32_t baudrate_get = 0;
  TEST_ASSERT_EQUAL(ESP_OK, uart_get_baudrate(uart_port, &baudrate_get));

  uint8_t delta = 10;
  TEST_ASSERT_UINT32_WITHIN(delta, baudrate_get, baud_rate);
}

//====================================================================================================================================

TEST_CASE("Check initialized data bits length", "[uart][init]") {
  uart_word_length_t data_bit_get;
  TEST_ASSERT_EQUAL(ESP_OK, uart_get_word_length(uart_port, &data_bit_get));
  TEST_ASSERT_EQUAL(data_bit_get, uart_data_bits);
}

//====================================================================================================================================

TEST_CASE("Check initialized parity", "[uart][init]") {
  uart_parity_t parity_mode_get;
  TEST_ASSERT_EQUAL(ESP_OK, uart_get_parity(uart_port, &parity_mode_get));
  TEST_ASSERT_EQUAL(parity_mode_get, uart_parity);
}

//====================================================================================================================================

TEST_CASE("Check initialized stop bits", "[uart][init]") {
  uart_stop_bits_t stop_bits_get;
  TEST_ASSERT_EQUAL(ESP_OK, uart_get_stop_bits(uart_port, &stop_bits_get));
  TEST_ASSERT_EQUAL(stop_bits_get, uart_stop_bits);
}

//====================================================================================================================================

TEST_CASE("Check initialized hardware flow control", "[uart][init]") {
  uart_hw_flowcontrol_t flow_ctrl_get;
  TEST_ASSERT_EQUAL(ESP_OK, uart_get_hw_flow_ctrl(uart_port, &flow_ctrl_get));
  TEST_ASSERT_EQUAL(flow_ctrl_get, hw_flow_ctrl);
}

//====================================================================================================================================

TEST_CASE("Check UART driver installation", "[uart][init]") {
  const int uart_port = UART_NUM_2;

  // Assertions
  TEST_ASSERT_TRUE(uart_is_driver_installed(uart_port));
}

//====================================================================================================================================

TEST_CASE("Test the receive multiple bytes function", "[uart][receive]") {
  int buffer_len = 32;
  uint8_t *receive_data_buffer = malloc(buffer_len);
  int bytes_received = 0;
  int receive_time_out = 20; // ms
  int ret = uart_receive_data(uart_port, receive_data_buffer, buffer_len,
                              receive_time_out, &bytes_received);

  TEST_ASSERT_EQUAL(ret, UART_DATA_RECEIVE_SUCCESS);
  TEST_ASSERT_EQUAL(0, bytes_received);
}

//====================================================================================================================================

TEST_CASE("Test receive function with multiple bytes sent", "[uart][receive]") {
  int buffer_len = 32;
  uint8_t *receive_data_buffer = malloc(buffer_len);
  int bytes_received;
  int receive_time_out = 20; // ms

  // uart send function
  uint8_t *send_data_buffer = malloc(buffer_len);
  int send_data_length = 3;
  memcpy(send_data_buffer, "ABC", send_data_length);

  int bytes_written;
  uart_send_data(uart_port, send_data_buffer, send_data_length, &bytes_written);

  int ret = uart_receive_data(uart_port, receive_data_buffer, buffer_len,
                              receive_time_out, &bytes_received);

  TEST_ASSERT_EQUAL(ret, UART_DATA_RECEIVE_SUCCESS);
  TEST_ASSERT_EQUAL(send_data_length, bytes_received);
}

//====================================================================================================================================

TEST_CASE("Test receive function with invalid pointer", "[uart][receive]") {
  int buffer_len = 32;
  uint8_t *receive_data_buffer = malloc(buffer_len);
  int receive_time_out = 20; // ms

  // uart send function
  uint8_t *send_data_buffer = malloc(buffer_len);
  int send_data_length = 3;
  memcpy(send_data_buffer, "ABC", send_data_length);

  int bytes_written;
  uart_send_data(uart_port, send_data_buffer, send_data_length, &bytes_written);

  int ret = uart_receive_data(uart_port, receive_data_buffer, buffer_len,
                              receive_time_out, NULL);

  TEST_ASSERT_EQUAL(ret, INVALID_POINTER);
}

//====================================================================================================================================

TEST_CASE("Test receive function with invalid data pointer",
          "[uart][receive]") {
  int buffer_len = 32;
  int bytes_received;
  int receive_time_out = 20; // ms

  // uart send function
  uint8_t *send_data_buffer = malloc(buffer_len);
  int send_data_length = 3;
  memcpy(send_data_buffer, "ABC", send_data_length);

  int bytes_written;
  uart_send_data(uart_port, send_data_buffer, send_data_length, &bytes_written);

  int ret = uart_receive_data(uart_port, NULL, buffer_len, receive_time_out,
                              &bytes_received);

  TEST_ASSERT_EQUAL(ret, INVALID_DATA_BUFFER_POINTER);
}

//====================================================================================================================================

TEST_CASE("Test receive function with invalid buffer length",
          "[uart][receive]") {
  int buffer_len = 32;
  uint8_t *receive_data_buffer = malloc(buffer_len);
  int bytes_received;
  int receive_time_out = 20; // ms

  // uart send function
  uint8_t *send_data_buffer = malloc(buffer_len);
  int send_data_length = 3;
  memcpy(send_data_buffer, "ABC", send_data_length);

  int bytes_written;
  uart_send_data(uart_port, send_data_buffer, send_data_length, &bytes_written);

  int invalid_buffer_len = 6000;
  int ret =
      uart_receive_data(uart_port, receive_data_buffer, invalid_buffer_len,
                        receive_time_out, &bytes_received);

  TEST_ASSERT_EQUAL(ret, INVALID_UART_RECEIVE_BUFFER_SIZE);
}

//====================================================================================================================================

TEST_CASE("Send function - Success", "[uart][send]") {
  int buffer_len = 32;
  uint8_t *receive_data_buffer = malloc(buffer_len);
  int bytes_received;
  int bytes_sent;
  int receive_time_out = 20; // ms

  // flush
  uart_flush(uart_port);

  // uart send function
  uint8_t *send_data_buffer = malloc(buffer_len);
  int send_data_length = 3;
  memcpy(send_data_buffer, "ABC", send_data_length);

  int ret = uart_send_data(uart_port, send_data_buffer, send_data_length,
                           &bytes_sent);
  uart_receive_data(uart_port, receive_data_buffer, buffer_len,
                    receive_time_out, &bytes_received);

  TEST_ASSERT_EQUAL(bytes_received, send_data_length);
  TEST_ASSERT_EQUAL(bytes_sent, send_data_length);
  TEST_ASSERT_EQUAL(ret, UART_DATA_SEND_SUCCESS);
}

//====================================================================================================================================

TEST_CASE("Send function - Inavlid data pointer", "[uart][send]") {
  int buffer_len = 32;
  uint8_t *receive_data_buffer = malloc(buffer_len);
  int bytes_received;
  int bytes_sent;
  int receive_time_out = 20; // ms
  int send_data_length = 3;
  // flush
  uart_flush(uart_port);

  // uart send function
  int ret = uart_send_data(uart_port, NULL, send_data_length, &bytes_sent);
  uart_receive_data(uart_port, receive_data_buffer, buffer_len,
                    receive_time_out, &bytes_received);

  TEST_ASSERT_EQUAL(ret, INVALID_DATA_BUFFER_POINTER);
}

//====================================================================================================================================

TEST_CASE("Send function - Invalid pointer to number_of_bytes_written variable",
          "[uart][send]") {
  int buffer_len = 32;
  uint8_t *receive_data_buffer = malloc(buffer_len);
  int bytes_received;
  int receive_time_out = 20; // ms

  // flush
  uart_flush(uart_port);

  // uart send function
  uint8_t *send_data_buffer = malloc(buffer_len);
  int send_data_length = 3;
  memcpy(send_data_buffer, "ABC", send_data_length);

  int ret = uart_send_data(uart_port, send_data_buffer, send_data_length, NULL);
  uart_receive_data(uart_port, receive_data_buffer, buffer_len,
                    receive_time_out, &bytes_received);

  TEST_ASSERT_EQUAL(ret, INVALID_POINTER);
}

//====================================================================================================================================

TEST_CASE("Send function - Invalid buffer size", "[uart][send]") {
  int buffer_len = 32;
  uint8_t *receive_data_buffer = malloc(buffer_len);
  int bytes_received;
  int bytes_sent;
  int receive_time_out = 20; // ms

  // flush
  uart_flush(uart_port);

  // uart send function
  uint8_t *send_data_buffer = malloc(buffer_len);
  int send_data_length = 3;
  memcpy(send_data_buffer, "ABC", send_data_length);

  int invalid_buffer_len = 5000;
  int ret = uart_send_data(uart_port, send_data_buffer, invalid_buffer_len,
                           &bytes_sent);
  uart_receive_data(uart_port, receive_data_buffer, buffer_len,
                    receive_time_out, &bytes_received);

  TEST_ASSERT_EQUAL(ret, INVALID_UART_SEND_BUFFER_SIZE);
}

//====================================================================================================================================

TEST_CASE("Send & Receive function - Success", "[uart][send_receive]") {
  int buffer_len = 32;
  uint8_t *receive_data_buffer = malloc(buffer_len);
  int receive_time_out = 20; // ms

  // flush
  uart_flush(uart_port);

  // uart send function
  uint8_t *send_data_buffer = malloc(buffer_len);
  int send_data_length = 3;
  memcpy(send_data_buffer, "ABC", send_data_length);

  int ret = uart_send_and_receive_data(uart_port, receive_data_buffer,
                                       buffer_len, send_data_buffer, buffer_len,
                                       receive_time_out);

  TEST_ASSERT_EQUAL(ret, ESP_OK);
  TEST_ASSERT_EQUAL(0,
                    memcmp(receive_data_buffer, send_data_buffer, buffer_len));
}

//====================================================================================================================================

TEST_CASE("Send & Receivesingle byte - Success",
          "[uart][send_receive_single_byte]") {
  int receive_time_out = 20; // ms

  // flush
  uart_flush(uart_port);

  uint8_t send_data = 'A';
  uint8_t receive_data;
  int ret = uart_send_single_byte(uart_port, &send_data);
  int recv_ret =
      uart_receive_single_byte(uart_port, &receive_data, receive_time_out);

  TEST_ASSERT_EQUAL(ret, ESP_OK);
  TEST_ASSERT_EQUAL(recv_ret, ESP_OK);
  TEST_ASSERT_EQUAL(send_data, receive_data);
}

//====================================================================================================================================

TEST_CASE("Check uart driver installation after deinitializing the driver",
          "[uart][deinit]") {
  const int uart_port = UART_NUM_2;
  uart_deinit(uart_port);

  // Assertions
  TEST_ASSERT_FALSE(uart_is_driver_installed(uart_port));
}