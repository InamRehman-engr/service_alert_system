#include "i2c-dev.h"

/**
 * @brief Check if an I2C device is available on the bus.
 *
 * This function attempts to communicate with an I2C device at the given address
 * to check if it is available on the I2C bus. It sends a simple write command
 * to the device and checks for a response.
 *
 * @param[in] addr           The I2C device address.
 * @param[in] device         Pointer to the I2C device configuration.
 *
 * @return                   ESP_OK if the device is available, ESP_ERR_TIMEOUT
 * if not available, ESP_ERR_MUTEX_FAILED if failed to acquire the mutex within
 * the timeout.
 */
esp_err_t i2c_device_available(uint8_t addr, i2c_device_t *device) {
  // Attempt to acquire the I2C mutex with a timeout
  if (xSemaphoreTake(device->i2c_mutex, pdMS_TO_TICKS(I2C_MUTEX_TIMEOUT_MS)) ==
      pdTRUE)
  // Mutex acquired successfully
  {
    // Create an I2C command handle
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    // Start an I2C transaction
    i2c_master_start(cmd);
    // Write the device address with the write bit set (indicating a write
    // operation)
    i2c_master_write_byte(cmd, (addr << 1) | WRITE_BIT, true);
    // Stop the I2C transaction
    i2c_master_stop(cmd);
    // Execute the I2C transaction and wait for completion with a timeout
    esp_err_t ret =
        i2c_master_cmd_begin(device->port, cmd, 1000 / portTICK_PERIOD_MS);
    // Delete the I2C command handle to free resources
    i2c_cmd_link_delete(cmd);
    // Release the acquired mutex to allow other tasks to use the I2C bus
    xSemaphoreGive(device->i2c_mutex);
    // Return the result of the I2C transaction
    return ret;
  } else {
    // Failed to acquire the mutex within the specified timeout
    return ESP_ERR_MUTEX_FAILED;
  }
}

/**
 * @brief Send data to an I2C device.
 *
 * This function initiates communication with an I2C device, sends a sequence of
 * data bytes to the device, and takes care of the necessary start, stop, and
 * ACK/NACK conditions for the transaction.
 *
 * @param[in] addr           The I2C device address.
 * @param[in] data           Pointer to the data buffer to send.
 * @param[in] data_len       The number of bytes to send.
 * @param[in] device         Pointer to the I2C device configuration.
 *
 * @return                   ESP_OK if the operation was successful, otherwise
 * an error code.
 */

esp_err_t i2c_send(uint8_t addr, uint8_t *data, size_t data_len,
                   i2c_device_t *device) {
  // Attempt to acquire the I2C mutex with a timeout
  if (xSemaphoreTake(device->i2c_mutex, pdMS_TO_TICKS(I2C_MUTEX_TIMEOUT_MS)) ==
      pdTRUE)
  // Mutex acquired successfully
  {
    // Create an I2C command handle
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();

    // Start an I2C transaction
    i2c_master_start(cmd);

    // Write the device address with the write bit set (indicating a write
    // operation)
    i2c_master_write_byte(cmd, (addr << 1) | WRITE_BIT, true);

    // Write the data to the I2C bus
    i2c_master_write(cmd, data, data_len, true);

    // Stop the I2C transaction
    i2c_master_stop(cmd);

    // Execute the I2C transaction and wait for completion with a timeout
    esp_err_t ret =
        i2c_master_cmd_begin(device->port, cmd, 1000 / portTICK_PERIOD_MS);

    // Delete the I2C command handle to free resources
    i2c_cmd_link_delete(cmd);

    // Release the acquired mutex to allow other tasks to use the I2C bus
    xSemaphoreGive(device->i2c_mutex);

    // Return the result of the I2C transaction
    return ret;
  } else {
    // Failed to acquire the mutex within the specified timeout
    return ESP_ERR_MUTEX_FAILED;
  }
}

/**
 * @brief Receive data from an I2C device.
 *
 * This function initiates communication with an I2C device, sends a read
 * request for a sequence of data bytes, and receives the data from the device.
 * It utilizes the I2C bus and takes care of the necessary start, stop, and
 * ACK/NACK conditions for the transaction.
 *
 * @param[in] addr           The I2C device address.
 * @param[out] data          Pointer to the buffer to store received data.
 * @param[in] data_len       The number of bytes to receive.
 * @param[in] device         Pointer to the I2C device configuration.
 *
 * @return                   ESP_OK if the operation was successful, otherwise
 * an error code.
 */
esp_err_t i2c_receive(uint8_t addr, uint8_t *data, size_t data_len,
                      i2c_device_t *device) {
  // Attempt to acquire the I2C mutex with a timeout
  if (xSemaphoreTake(device->i2c_mutex, pdMS_TO_TICKS(I2C_MUTEX_TIMEOUT_MS)) ==
      pdTRUE) {
    // Mutex acquired successfully

    // Create an I2C command handle
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();

    // Stop the I2C transaction
    i2c_master_start(cmd);

    // Write the device address with the read bit set (indicating a read
    // operation)
    i2c_master_write_byte(cmd, ((addr << 1) | READ_BIT), true);

    // Read data from the I2C bus into the provided buffer with the last byte
    // being Not Acknowledge
    i2c_master_read(cmd, data, data_len, I2C_MASTER_LAST_NACK);

    // Stop the I2C transaction
    i2c_master_stop(cmd);

    // Execute the I2C transaction and wait for completion with a timeout
    esp_err_t ret =
        i2c_master_cmd_begin(device->port, cmd, 1000 / portTICK_PERIOD_MS);

    // Delete the I2C command handle to free resources

    i2c_cmd_link_delete(cmd);
    // Release the acquired mutex to allow other tasks to use the I2C bus

    xSemaphoreGive(device->i2c_mutex);
    // Return the result of the I2C transaction
    return ret;
  } else {
    // Failed to acquire the mutex within the specified timeout
    return ESP_ERR_MUTEX_FAILED;
  }
}

/**
 * @brief Send and receive data to/from an I2C device in a single transaction.
 *
 * This function initiates communication with an I2C device, sends a sequence of
 * data bytes to the device, and then receives a sequence of data bytes from the
 * device. It utilizes the I2C bus and takes care of the necessary start, stop,
 * and ACK/NACK conditions for both sending and receiving data.
 *
 * @param[in] addr               The I2C device address.
 * @param[in] data_send          Pointer to the data buffer to send.
 * @param[in] data_send_len      The number of bytes to send.
 * @param[out] data_receive     Pointer to the buffer to store received data.
 * @param[in] data_receive_len   The number of bytes to receive.
 * @param[in] device            Pointer to the I2C device configuration.
 *
 * @return                      ESP_OK if the operation was successful,
 * otherwise an error code.
 */

esp_err_t i2c_send_receive(uint8_t addr, uint8_t *data_send,
                           size_t data_send_len, uint8_t *data_receive,
                           size_t data_receive_len, i2c_device_t *device) {
  // Attempt to acquire the I2C mutex with a timeout
  if (xSemaphoreTake(device->i2c_mutex, pdMS_TO_TICKS(I2C_MUTEX_TIMEOUT_MS)) ==
      pdTRUE) {
    // Mutex acquired successfully

    // Create an I2C command handle
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();

    // Start an I2C transaction for sending data
    i2c_master_start(cmd);

    // Write the device address with the write bit set (indicating a write
    // operation)
    i2c_master_write_byte(cmd, (addr << 1) | WRITE_BIT, true);

    // Write the data to the I2C bus
    i2c_master_write(cmd, data_send, data_send_len, true);

    // Start a new I2C transaction for receiving data
    i2c_master_start(cmd);

    // Write the device address with the read bit set (indicating a read
    // operation)
    i2c_master_write_byte(cmd, (addr << 1) | READ_BIT, true);

    // Read data from the I2C bus into the provided buffer with the last byte
    // being NACKed
    i2c_master_read(cmd, data_receive, data_receive_len, I2C_MASTER_LAST_NACK);

    // Stop the I2C transaction
    i2c_master_stop(cmd);

    // Execute the I2C transaction and wait for completion with a timeout
    esp_err_t ret =
        i2c_master_cmd_begin(device->port, cmd, 1000 / portTICK_PERIOD_MS);

    // Delete the I2C command handle to free resources
    i2c_cmd_link_delete(cmd);

    // Release the acquired mutex to allow other tasks to use the I2C bus
    xSemaphoreGive(device->i2c_mutex);

    // Return the result of the I2C transaction
    return ret;
  } else {
    // Failed to acquire the mutex within the specified timeout
    return ESP_ERR_MUTEX_FAILED;
  }
}

/**
 * @brief Send a single byte of data to an SMBus device.
 *
 * This function initiates communication with an SMBus device, sends a single
 * data byte to the device, and takes care of the necessary start, stop, and
 * ACK/NACK conditions for the transaction.
 *
 * @param[in] address   The I2C device address.
 * @param[in] data      The data byte to send.
 * @param[in] device    Pointer to the I2C device configuration.
 *
 * @return              ESP_OK if the operation was successful, otherwise an
 * error code.
 */
esp_err_t smbus_send_byte(uint8_t address, uint8_t data, i2c_device_t *device) {
  // Attempt to acquire the I2C mutex with a timeout
  if (xSemaphoreTake(device->i2c_mutex, pdMS_TO_TICKS(I2C_MUTEX_TIMEOUT_MS)) ==
      pdTRUE) {
    // Mutex acquired successfully

    // Create an I2C command handle
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();

    // Start an I2C transaction
    i2c_master_start(cmd);

    // Write the device address with the write bit set (indicating a write
    // operation)
    i2c_master_write_byte(cmd, address << 1 | WRITE_BIT, true);

    // Write the data byte to the I2C bus
    i2c_master_write_byte(cmd, data, true);

    // Stop the I2C transaction
    i2c_master_stop(cmd);

    // Execute the I2C transaction and wait for completion with a timeout
    esp_err_t ret =
        i2c_master_cmd_begin(device->port, cmd, 1000 / portTICK_PERIOD_MS);

    // Delete the I2C command handle to free resources
    i2c_cmd_link_delete(cmd);

    // Release the acquired mutex to allow other tasks to use the I2C bus
    xSemaphoreGive(device->i2c_mutex);

    // Return the result of the I2C transaction
    return ret;
  } else {
    // Failed to acquire the mutex within the specified timeout
    return ESP_ERR_MUTEX_FAILED;
  }
}

/**
 * @brief Receive a single byte from an SMBus device.
 *
 * This function initiates communication with an SMBus device, sends a read
 * request for a single byte, and receives the byte from the device. It utilizes
 * the I2C bus and takes care of the necessary start, stop, and ACK/NACK
 * conditions for the transaction.
 *
 * @param[in] address   The I2C device address.
 * @param[out] data     Pointer to store the received byte.
 * @param[in] device    Pointer to the I2C device configuration.
 *
 * @return              ESP_OK if the operation was successful, otherwise an
 * error code.
 */
esp_err_t smbus_receive_byte(uint8_t address, uint8_t *data,
                             i2c_device_t *device) {
  // Attempt to acquire the I2C mutex with a timeout
  if (xSemaphoreTake(device->i2c_mutex, pdMS_TO_TICKS(I2C_MUTEX_TIMEOUT_MS)) ==
      pdTRUE) {
    // Mutex acquired successfully

    // Create an I2C command handle
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();

    // Start an I2C transaction
    i2c_master_start(cmd);

    // Write the device address with the read bit set (indicating a read
    // operation) and request ACK
    i2c_master_write_byte(cmd, address << 1 | READ_BIT, ACK_CHECK);

    // Read a single byte of data from the I2C bus with NACK
    i2c_master_read_byte(cmd, data, NACK_VALUE);

    // Stop the I2C transaction
    i2c_master_stop(cmd);

    // Execute the I2C transaction and wait for completion with a timeout
    esp_err_t ret =
        i2c_master_cmd_begin(device->port, cmd, 1000 / portTICK_PERIOD_MS);

    // Delete the I2C command handle to free resources
    i2c_cmd_link_delete(cmd);

    // Release the acquired mutex to allow other tasks to use the I2C bus
    xSemaphoreGive(device->i2c_mutex);

    // Return the result of the I2C transaction
    return ret;
  } else {
    // Failed to acquire the mutex within the specified timeout
    return ESP_ERR_MUTEX_FAILED;
  }
}

/**
 * @brief Send a sequence of bytes to an SMBus device.
 *
 * This function initiates communication with an SMBus device, sends a command
 * byte and a sequence of data bytes to the device. It utilizes the I2C bus and
 * takes care of the necessary start, stop, and ACK/NACK conditions for the
 * transaction.
 *
 * @param[in] address   The I2C device address.
 * @param[in] command   The command byte to send before transmitting data.
 * @param[in] data      Pointer to the data buffer to send.
 * @param[in] len       The number of bytes to send.
 * @param[in] device    Pointer to the I2C device configuration.
 *
 * @return              ESP_OK if the operation was successful, otherwise an
 * error code.
 */
esp_err_t smbus_send_bytes(uint8_t address, uint8_t command, uint8_t *data,
                           size_t len, i2c_device_t *device) {
  // Attempt to acquire the I2C mutex with a timeout
  if (xSemaphoreTake(device->i2c_mutex, pdMS_TO_TICKS(I2C_MUTEX_TIMEOUT_MS)) ==
      pdTRUE) {
    // Mutex acquired successfully

    // Create an I2C command handle
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();

    // Start an I2C transaction
    i2c_master_start(cmd);

    // Write the device address with the write bit set (indicating a write
    // operation) and request ACK
    i2c_master_write_byte(cmd, address << 1 | WRITE_BIT, ACK_CHECK);

    // Write the command byte to the I2C bus with ACK
    i2c_master_write_byte(cmd, command, ACK_CHECK);

    // Write the data bytes to the I2C bus with ACK for each byte
    i2c_master_write(cmd, data, len, ACK_CHECK);

    // Stop the I2C transaction
    i2c_master_stop(cmd);

    // Execute the I2C transaction and wait for completion with a timeout
    esp_err_t ret =
        i2c_master_cmd_begin(device->port, cmd, 1000 / portTICK_PERIOD_MS);

    // Delete the I2C command handle to free resources
    i2c_cmd_link_delete(cmd);

    // Release the acquired mutex to allow other tasks to use the I2C bus
    xSemaphoreGive(device->i2c_mutex);

    // Return the result of the I2C transaction
    return ret;
  } else {
    // Failed to acquire the mutex within the specified timeout
    return ESP_ERR_MUTEX_FAILED;
  }
}

/**
 * @brief Receive a sequence of bytes from an SMBus device.
 *
 * This function initiates communication with an SMBus device, sends a command
 * byte, and receives a sequence of bytes from the device. It utilizes the I2C
 * bus and takes care of the necessary start, stop, and ACK/NACK conditions for
 * the transaction.
 *
 * @param[in] address   The I2C device address.
 * @param[in] command   The command byte to send before receiving data.
 * @param[out] data     Pointer to the buffer to store received data.
 * @param[in] len       The number of bytes to receive.
 * @param[in] device    Pointer to the I2C device configuration.
 *
 * @return              ESP_OK if the operation was successful, otherwise an
 * error code.
 */
esp_err_t smbus_receive_bytes(uint8_t address, uint8_t command, uint8_t *data,
                              size_t len, i2c_device_t *device) {
  // Attempt to acquire the I2C mutex with a timeout
  if (xSemaphoreTake(device->i2c_mutex, pdMS_TO_TICKS(I2C_MUTEX_TIMEOUT_MS)) ==
      pdTRUE) {
    // Mutex acquired successfully

    // Create an I2C command handle
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();

    // Start an I2C transaction
    i2c_master_start(cmd);

    // Write the device address with the write bit set (indicating a write
    // operation) and request ACK
    i2c_master_write_byte(cmd, address << 1 | WRITE_BIT, ACK_CHECK);

    // Write the command byte to the I2C bus with ACK
    i2c_master_write_byte(cmd, command, ACK_CHECK);

    // Start a new I2C transaction
    i2c_master_start(cmd);

    // Write the device address with the read bit set (indicating a read
    // operation) and request ACK
    i2c_master_write_byte(cmd, address << 1 | READ_BIT, ACK_CHECK);

    // Read data from the I2C bus into the provided buffer with ACK for all but
    // the last byte
    if (len > 1) {
      i2c_master_read(cmd, data, len - 1, ACK_VALUE);
    }

    // Read the last byte of data from the I2C bus with NACK
    i2c_master_read_byte(cmd, &data[len - 1], NACK_VALUE);

    // Stop the I2C transaction
    i2c_master_stop(cmd);

    // Execute the I2C transaction and wait for completion with a timeout
    esp_err_t ret =
        i2c_master_cmd_begin(device->port, cmd, 1000 / portTICK_PERIOD_MS);

    // Delete the I2C command handle to free resources
    i2c_cmd_link_delete(cmd);

    // Release the acquired mutex to allow other tasks to use the I2C bus
    xSemaphoreGive(device->i2c_mutex);

    // Return the result of the I2C transaction
    return ret;
  } else {
    // Failed to acquire the mutex within the specified timeout
    return ESP_ERR_MUTEX_FAILED;
  }
}

/**
 * @brief Sends a block of data over the SMBus protocol.
 *
 * This function sends a block of data over the I2C bus using the SMBus
 * protocol.
 *
 * @param[in] address   The I2C device address.
 * @param[in] command   The command byte to send.
 * @param[in] data      Pointer to the data buffer to send.
 * @param[in] len       The number of bytes to send.
 * @param[in] device    Pointer to the I2C device configuration.
 *
 * @return              ESP_OK if the operation was successful, otherwise an
 * error code.
 */
esp_err_t smbus_send_block(uint8_t address, uint8_t command, uint8_t *data,
                           size_t len, i2c_device_t *device) {
  // Attempt to acquire the I2C mutex with a timeout
  if (xSemaphoreTake(device->i2c_mutex, pdMS_TO_TICKS(I2C_MUTEX_TIMEOUT_MS)) ==
      pdTRUE) {
    // Mutex acquired successfully

    // Create an I2C command handle
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();

    // Start an I2C transaction
    i2c_master_start(cmd);

    // Write the device address with the write bit set (indicating a write
    // operation) and request ACK
    i2c_master_write_byte(cmd, address << 1 | WRITE_BIT, ACK_CHECK);

    // Write the command byte to the I2C bus with ACK
    i2c_master_write_byte(cmd, command, ACK_CHECK);

    // Write the length byte to specify the number of data bytes to follow, and
    // request ACK
    i2c_master_write_byte(cmd, len, ACK_CHECK);

    // Write the data bytes to the I2C bus with ACK for each byte
    for (size_t i = 0; i < len; ++i) {
      i2c_master_write_byte(cmd, data[i], ACK_CHECK);
    }

    // Stop the I2C transaction
    i2c_master_stop(cmd);

    // Execute the I2C transaction and wait for completion with a timeout
    esp_err_t ret =
        i2c_master_cmd_begin(device->port, cmd, 1000 / portTICK_PERIOD_MS);

    // Delete the I2C command handle to free resources
    i2c_cmd_link_delete(cmd);

    // Release the acquired mutex to allow other tasks to use the I2C bus
    xSemaphoreGive(device->i2c_mutex);

    // Return the result of the I2C transaction
    return ret;
  } else {
    // Failed to acquire the mutex within the specified timeout
    return ESP_ERR_MUTEX_FAILED;
  }
}
/**
 * @brief Receives a block of data over the SMBus protocol.
 *
 * This function receives a block of data over the I2C bus using the SMBus
 * protocol.
 *
 * @param[in] address   The I2C device address.
 * @param[in] command   The command byte to send before receiving data.
 * @param[out] data     Pointer to the buffer to store received data.
 * @param[in,out] len   On input, specifies the maximum length of the buffer; on
 * output, receives the actual received length.
 * @param[in] device    Pointer to the I2C device configuration.
 *
 * @return              ESP_OK if the operation was successful, otherwise an
 * error code.
 */
esp_err_t smbus_receive_block(uint8_t address, uint8_t command, uint8_t *data,
                              uint8_t len, i2c_device_t *device) {
  // Attempt to acquire the I2C mutex with a timeout
  if (xSemaphoreTake(device->i2c_mutex, pdMS_TO_TICKS(I2C_MUTEX_TIMEOUT_MS)) ==
      pdTRUE) {
    // Mutex acquired successfully

    i2c_cmd_handle_t cmd = i2c_cmd_link_create();

    // Start an I2C transaction
    i2c_master_start(cmd);

    // Write the device address with the write bit set (indicating a write
    // operation) and request ACK
    i2c_master_write_byte(cmd, address << 1 | WRITE_BIT, ACK_CHECK);

    // Write the command byte to the I2C bus with ACK
    i2c_master_write_byte(cmd, command, ACK_CHECK);

    // Start a new I2C transaction
    i2c_master_start(cmd);

    // Write the device address with the read bit set (indicating a read
    // operation) and request ACK
    i2c_master_write_byte(cmd, address << 1 | READ_BIT, ACK_CHECK);

    // Read the length byte from the I2C bus with ACK
    uint8_t slave_len = 0;
    i2c_master_read_byte(cmd, &slave_len, ACK_VALUE);

    // Execute the I2C transaction and wait for completion with a timeout
    esp_err_t ret =
        i2c_master_cmd_begin(device->port, cmd, 1000 / portTICK_PERIOD_MS);
    i2c_cmd_link_delete(cmd);

    // Check if the operation was successful
    if (ret != ESP_OK) {
      // *len = 0;
      return ret;
    }

    // Limit the received length to the maximum specified length
    if (slave_len > len) {
      ESP_LOGW("I2C", "slave data length %d exceeds data len %d bytes",
               slave_len, len);
      slave_len = len;
    }

    cmd = i2c_cmd_link_create();

    // Read data bytes from the I2C bus with ACK for all but the last byte
    for (size_t i = 0; i < slave_len - 1; ++i) {
      i2c_master_read_byte(cmd, &data[i], ACK_VALUE);
    }

    // Read the last byte of data from the I2C bus with NACK
    i2c_master_read_byte(cmd, &data[slave_len - 1], NACK_VALUE);

    // Stop the I2C transaction
    i2c_master_stop(cmd);

    // Execute the I2C transaction and wait for completion with a timeout
    ret = i2c_master_cmd_begin(device->port, cmd, 1000 / portTICK_PERIOD_MS);
    i2c_cmd_link_delete(cmd);

    // Update the actual received length
    // if (ret == ESP_OK) {
    //     *len = slave_len;
    // } else {
    //     *len = 0;
    // }

    // Release the acquired mutex to allow other tasks to use the I2C bus
    xSemaphoreGive(device->i2c_mutex);

    // Return the result of the I2C transaction
    return ret;
  } else {
    // Failed to acquire the mutex within the specified timeout
    return ESP_ERR_MUTEX_FAILED;
  }
}

esp_err_t smbus_send_word(uint8_t address, uint8_t command, uint8_t *data,
                          i2c_device_t *device) {
  return smbus_send_bytes(address, command, data, 2, device);
}

esp_err_t smbus_receive_word(uint8_t address, uint8_t command, uint8_t *data,
                             i2c_device_t *device) {
  return smbus_receive_bytes(address, command, data, 2, device);
}

/**
 * @brief Initializes the I2C interface with the specified configuration.
 *
 * This function initializes the I2C interface with the specified configuration
 * and assigns I2C-related functions to the provided structure.
 *
 * @param[in] mode              The I2C mode (master/slave).
 * @param[in] sda_io            The GPIO pin number for the SDA (data) line.
 * @param[in] scl_io            The GPIO pin number for the SCL (clock) line.
 * @param[in] sda_pullup_en     Enable SDA pull-up resistor.
 * @param[in] scl_pullup_en     Enable SCL pull-up resistor.
 * @param[in] clk_speed         The clock speed of the I2C bus.
 * @param[in] functions         Pointer to a structure holding I2C-related
 * function pointers.
 */
void i2c_init(i2c_mode_t mode, int sda_io, int scl_io, bool sda_pullup_en,
              bool scl_pullup_en, uint32_t clk_speed,
              i2c_functions *functions) {
  // Initialize I2C configuration
  i2c_config_t configs = {
      .mode = mode,
      .sda_io_num = sda_io,
      .scl_io_num = scl_io,
      .sda_pullup_en = sda_pullup_en,
      .scl_pullup_en = scl_pullup_en,
      .master.clk_speed = clk_speed,
  };

  // Create an I2C mutex for synchronization
  functions->device->i2c_mutex = xSemaphoreCreateMutex();

  // Assign I2C-related functions to the provided structure
  functions->i2c_send = i2c_send;
  functions->i2c_receive = i2c_receive;
  functions->i2c_send_receive = i2c_send_receive;
  functions->device_available = i2c_device_available;

#ifdef CONFIG_SMBUS_SUPPORT
  functions->smbus_send_byte = smbus_send_byte;
  functions->smbus_receive_byte = smbus_receive_byte;
  functions->smbus_send_bytes = smbus_send_bytes;
  functions->smbus_receive_bytes = smbus_receive_bytes;
  functions->smbus_send_block = smbus_send_block;
  functions->smbus_receive_block = smbus_receive_block;
  functions->smbus_send_word = smbus_send_word;
  functions->smbus_receive_word = smbus_receive_word;
#endif

  // Configure I2C parameters and install the driver
  i2c_param_config(functions->device->port, &configs);
  i2c_driver_install(functions->device->port, configs.mode,
                     I2C_MASTER_RX_BUF_DISABLE, I2C_MASTER_TX_BUF_DISABLE, 0);
}
