#ifndef __UART_DRIVER_H__
#define __UART_DRIVER_H__

#include "driver/gpio.h"
#include "driver/uart.h"
#include "esp_err.h"
#include "sdkconfig.h"
#include <stdio.h>
#include <string.h>

extern const int MAX_SEND_BUFFER_LENGTH;
extern const int MAX_RECEIVE_BUFFER_LENGTH;

// Errors ENUM
typedef enum {
  SUCCESS,
  INVALID_DATA_BUFFER_POINTER,
  INVALID_UART_SEND_BUFFER_SIZE,
  UART_DATA_SEND_SUCCESS,
  UART_DATA_RECEIVE_SUCCESS,
  UART_DATA_SEND_ERROR,
  UART_DATA_RECEIVE_ERROR,
  INVALID_POINTER,
  INVALID_UART_RECEIVE_BUFFER_SIZE,
  UART_FAIL
} uart_error_codes;

// struct containing Pins for UART
typedef struct {
  gpio_num_t TXD;
  gpio_num_t RXD;
  gpio_num_t CTS;
  gpio_num_t RTS;
} uart_pin_config_t;

// Structure containing function pointers for uart.
typedef struct {
  /**
   * @brief UART event queue handle (out param). On success of the uart_init, a
   * new queue handle is written here to provide access to UART events. If set
   * to NULL, driver will not use an event queue and event based data gathering
   * will not function.
   */
  QueueHandle_t uart_queue;

  /**
   * @brief Receive data from the specified UART port.
   *
   * This function reads data from the UART port specified by `uart_port_no` and
   * stores it in the provided data buffer (`data`). The maximum number of bytes
   * to receive is determined by `receive_buffer_size`. The function will wait
   * for data to be received for a maximum duration specified by
   * `uart_receive_timeout_ms`.
   *
   * @param[in] uart_queue queue handle for UART Queue.
   * @param[in] uart_port_no UART port number from which to receive data.
   * @param[out] data Pointer to the buffer where received data will be stored.
   * @param[in] receive_buffer_size Size of the receive buffer (maximum number
   * of bytes to receive).
   * @param[in] uart_receive_timeout_ticks Maximum time to wait for data
   * reception (in milliseconds).
   * @param[out] number_of_bytes_received Pointer to an integer where the actual
   * number of received bytes will be stored.
   *
   * @return uart_error_codes : UART_DATA_RECEIVE_SUCCESS if data is
   * successfully received, UART_DATA_RECEIVE_ERROR if an error occurs during
   * reception, or INVALID_DATA_BUFFER_POINTER if the data pointer is NULL.
   *
   * @note The function returns UART_DATA_RECEIVE_SUCCESS even if fewer bytes
   * are received than specified in `receive_buffer_size`. Check the
   * `number_of_bytes_received` parameter to determine the actual number of
   * bytes received.
   *
   */
  uart_error_codes (*uart_receive_data)(QueueHandle_t *uart_queue,
                                        uart_port_t uart_port_no, uint8_t *data,
                                        int receive_buffer_size,
                                        TickType_t uart_receive_timeout_ticks,
                                        int *number_of_bytes_received);

  /**
   * @brief Send data over the specified UART port.
   *
   * This function sends the specified data buffer (`data`) of the given length
   * (`data_len`) over the UART port specified by `uart_port_no`. It also
   * provides the number of bytes successfully written via the output parameter
   * `number_of_bytes_written`.
   *
   * @param[in] uart_port_no UART port number to send data.
   * @param[in] data Pointer to the data buffer to be sent.
   * @param[in] data_len Length of the data to be sent (in bytes).
   * @param[out] number_of_bytes_written Pointer to an integer where the actual
   * number of bytes written will be stored.
   *
   * @return
   *   - `UART_DATA_SEND_SUCCESS` if data is successfully sent.
   *   - `UART_DATA_SEND_ERROR` if an error occurs during data transmission.
   *   - `INVALID_DATA_BUFFER_POINTER` if the data pointer is NULL.
   *   - `INVALID_UART_SEND_BUFFER_SIZE` if the data length is invalid (must be
   * greater than 0 and less than or equal to 1024).
   *
   * @note The function sets the error code and logs an error message using
   * ESP_LOGE if an error occurs during transmission. Make sure to handle the
   * error code appropriately in the calling code.
   */
  uart_error_codes (*uart_send_data)(uart_port_t uart_port_no, uint8_t *data,
                                     size_t data_len,
                                     int *number_of_bytes_written);

  /**
   * @brief Send data over the specified UART port and simultaneously receive
   * data.
   *
   * This function sends a specified data buffer (`data_send_buff`) of a given
   * length (`send_data_len`) over the UART port specified by `uart_port_no`. It
   * also receives data into the buffer `data_receive_buff` with a specified
   * length (`receive_data_len`). The function waits for data reception for up
   * to the specified `uart_receive_timeout_ms` milliseconds.
   *
   * @param[in] uart_queue queue handle for UART Queue.
   * @param[in] uart_port_no UART port number to send and receive data.
   * @param[out] data_receive_buff Pointer to the buffer where received data
   * will be stored.
   * @param[in] receive_buffer_size Length of the data to be received (in
   * bytes).
   * @param[in] data_send_buff Pointer to the data buffer to be sent.
   * @param[in] send_data_len Length of the data to be sent (in bytes).
   * @param[in] uart_receive_timeout_ticks Maximum time to wait for data
   * reception (in milliseconds).
   *
   * @return
   *   - `ESP_OK` if data is successfully sent and received.
   *   - `ESP_FAIL` if an error occurs during data transmission or reception.
   *
   * @note
   * - The function internally uses `uart_send_data` and `uart_receive_data`
   * functions for sending and receiving data.
   * - Ensure proper initialization of the UART port before calling this
   * function.
   * - Proper memory allocation for `data_receive_buff` and `data_send_buff` is
   * essential to prevent buffer overflows.
   * - The function logs success messages after successful data transmission and
   * reception.
   *
   * Example usage:
   * @code{.c}
   * uint8_t send_buffer[] = {0x01, 0x02, 0x03}; // Example send data buffer
   * uint8_t receive_buffer[3]; // Buffer to store received data
   * esp_err_t result = uart_send_and_receive_data(UART_NUM_1, receive_buffer,
   * sizeof(receive_buffer), send_buffer, sizeof(send_buffer),
   * uart_receive_timeout_ms); if (result == ESP_OK) {
   *     // Data sent and received successfully, use receive_buffer for further
   * processing. } else {
   *     // Error occurred during transmission or reception.
   * }
   * @endcode
   */
  esp_err_t (*uart_send_and_receive_data)(
      QueueHandle_t *uart_queue, uart_port_t uart_port_no,
      uint8_t *data_receive_buff, int receive_buffer_size,
      uint8_t *data_send_buff, int send_data_len,
      TickType_t uart_receive_timeout_ticks);

  /**
   * @brief Send a single byte over the specified UART port.
   *
   * This function sends a single byte of data over the UART port specified by
   * `uart_port_no`.
   *
   * @param[in] uart_port_no UART port number to send the byte.
   * @param[in] data Pointer to the single byte of data to be sent.
   *
   * @return
   *   - `ESP_OK` if the byte is successfully sent.
   *   - `ESP_FAIL` if an error occurs during data transmission.
   *
   * @note
   * - The function internally uses `uart_send_data` to transmit the byte.
   * - Ensure proper initialization of the UART port before calling this
   * function.
   * - The function logs a success message if the byte is sent successfully.
   * - Error handling is performed inside the function. If `uart_send_data`
   * fails to send the byte, `ESP_FAIL` is returned, and an error message is
   * logged.
   *
   * Example usage:
   * @code{.c}
   * uint8_t byte_to_send = 0x55; // Example byte to be sent
   * esp_err_t send_result = uart_send_single_byte(UART_NUM_1, &byte_to_send);
   * if (send_result == ESP_OK) {
   *     // Byte sent successfully
   * } else {
   *     // Error occurred during transmission
   * }
   * @endcode
   */
  esp_err_t (*uart_send_single_byte)(uart_port_t uart_port_no, uint8_t *data);

  /**
   * @brief Receive a single byte of data over the specified UART port.
   *
   * This function receives a single byte of data over the UART port specified
   * by `uart_port_no`.
   *
   * @param[in] uart_queue queue handle for UART Queue.
   * @param[in] uart_port_no UART port number to receive the byte.
   * @param[out] data Pointer to the memory location where the received byte
   * will be stored.
   * @param[in] uart_receive_timeout_ticks Maximum time to wait for data
   * reception (in milliseconds).
   *
   * @return
   *   - `ESP_OK` if the byte is successfully received.
   *   - `ESP_FAIL` if an error occurs during data reception or if the reception
   * timeout is reached.
   *
   * @note
   * - The function internally uses `uart_receive_data` to retrieve the byte.
   * - Ensure proper initialization of the UART port before calling this
   * function.
   * - The function logs a success message if the byte is received successfully.
   * - Error handling is performed inside the function. If `uart_receive_data`
   * fails to receive the byte, `ESP_FAIL` is returned, and an error message is
   * logged.
   *
   * Example usage:
   * @code{.c}
   * uint8_t received_byte = 0;
   * esp_err_t receive_result = uart_receive_single_byte(UART_NUM_1,
   * &received_byte, uart_receive_timeout_ms); if (receive_result == ESP_OK) {
   *     // Byte received successfully, use received_byte for further
   * processing. } else {
   *     // Error occurred during reception or timeout reached.
   * }
   * @endcode
   */
  esp_err_t (*uart_receive_single_byte)(QueueHandle_t *uart_queue,
                                        uart_port_t uart_port_no, uint8_t *data,
                                        TickType_t uart_receive_timeout_ticks);

  /**
   * @brief Deinitialize the specified UART port.
   *
   * This function deinitialize the UART communication on the specified UART
   * port. It deletes the UART driver associated with the port, freeing up
   * system resources.
   *
   * @param[in] uart_port UART port number to deinitialize.
   *
   * @note
   * - Ensure that the UART port is properly initialized before calling this
   * function.
   * - This function internally calls `uart_driver_delete` to remove the UART
   * driver.
   * - Error checks are performed using ESP_ERROR_CHECK to ensure proper
   * deinitialization.
   *
   * Example usage:
   * @code{.c}
   * uart_deinit(UART_NUM_1);
   * @endcode
   */
  void (*uart_deinit)(uart_port_t);

} uart_peripheral;

//================================================================================================================================================================

/**
 * @brief Initialize UART communication with the specified configuration and pin
 * settings.
 *
 * This function initializes the UART communication on the specified UART port
 * with the provided configuration settings and pin configurations.
 *
 * @param[in] uart_inst instance for the uart peripheral.
 * @param[in] uart_port_num UART port number to initialize.
 * @param[in] uart_config Pointer to the UART configuration structure.
 * @param[in] pin_config UART pin configuration structure containing TXD, RXD,
 * RTS, and CTS pin numbers.
 * @param[in] mode UART transaction mode.
 * @param[in] event_queue_size UART event queue size/depth.
 * @param[in] tx_buffer_size UART transmit buffer size (in bytes).
 * @param[in] rx_buffer_size UART receive buffer size (in bytes).
 * @return on success of driver install function will return true.
 *
 * @note
 * - Ensure proper hardware initialization and configuration of `uart_config`
 * and `pin_config` structures before calling this function.
 * - The function performs error checks and logs error messages using
 * ESP_ERROR_CHECK, ensuring proper initialization.
 * - Proper memory allocation for buffers (`tx_buffer_size` and
 * `rx_buffer_size`) is essential to prevent buffer overflows.
 *
 * Example usage:
 * @code{.c}
 * uart_peripheral uart_inst;
 * uart_config_t uart_config = {
 *     .baud_rate = 115200,
 *     .data_bits = UART_DATA_8_BITS,
 *     .parity = UART_PARITY_DISABLE,
 *     .stop_bits = UART_STOP_BITS_1,
 *     .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
 *     .source_clk = UART_SCLK_APB,
 * };
 *
 * uart_pin_config_t pin_config = {
 *     .TXD = GPIO_NUM_1,
 *     .RXD = GPIO_NUM_3,
 *     .CTS = GPIO_NUM_5,
 *     .RTS = GPIO_NUM_7,
 * };
 *
 * uart_init(uart_inst ,UART_NUM_1, &uart_config, pin_config, UART_MODE_UART,
 * EVENT_QUEUE_SIZE, TX_BUFFER_SIZE, RX_BUFFER_SIZE);
 * @endcode
 */
bool uart_init(uart_peripheral *uart_inst, uart_port_t uart_port_num,
               uart_config_t *uart_config, uart_pin_config_t pin_config,
               uart_mode_t mode, int event_queue_size, int tx_buffer_size,
               int rx_buffer_size);

#endif
