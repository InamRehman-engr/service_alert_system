#include "spi_master_dev.h"
#include "esp_log.h"

SemaphoreHandle_t spimutex[3];

esp_err_t spi_send(struct spi_master_functions *configs, uint16_t command,
                   uint64_t address, uint8_t *data, size_t len) {
  if (xSemaphoreTake(spimutex[configs->spi_host], pdMS_TO_TICKS(1000)) ==
      pdTRUE) {
    spi_transaction_t trans = {
        .rxlength = len * 8,
        .length = len * 8, // total length of data send and receive combined.
        .tx_buffer = data,
        .user = (void *)0,
    };
    if (command != 0) {
      trans.cmd = command;
    }
    if (address != 0) {
      trans.addr = address;
    }
    esp_err_t ret = spi_device_polling_transmit(configs->spi_device, &trans);
    if (ret == ESP_OK) {
      ESP_LOGD("SPI DATA", "Data sent");
    } else {
      ESP_LOGE("SPI DATA", "Write error");
    }
    xSemaphoreGive(spimutex[configs->spi_host]);
    return ret;
  } else {
    return ESP_ERR_MUTEX_FAILED;
  }
}

esp_err_t spi_receive(struct spi_master_functions *configs, uint16_t command,
                      uint64_t address, uint8_t *data, size_t len) {
  if (xSemaphoreTake(spimutex[configs->spi_host], pdMS_TO_TICKS(1000)) ==
      pdTRUE) {
    spi_transaction_t trans = {
        .rxlength = len * 8,
        .length = len * 8, // total length of data send and receive combined.
        .tx_buffer = {0},
        .rx_buffer = data,
        .user = (void *)0,
    };
    if (command != 0) {
      trans.cmd = command;
    }
    if (address != 0) {
      trans.addr = address;
    }
    esp_err_t ret = spi_device_polling_transmit(configs->spi_device, &trans);
    if (ret == ESP_OK) {
      ESP_LOGD("SPI DATA", "Data received");
    } else {
      ESP_LOGE("SPI DATA", "Read error");
    }
    xSemaphoreGive(spimutex[configs->spi_host]);
    return ret;
  } else {
    return ESP_ERR_MUTEX_FAILED;
  }
}

esp_err_t spi_send_receive(struct spi_master_functions *configs,
                           uint16_t command, uint64_t address, uint8_t *tx_data,
                           uint8_t *rx_data, size_t len) {
  if (xSemaphoreTake(spimutex[configs->spi_host], pdMS_TO_TICKS(1000)) ==
      pdTRUE) {
    spi_transaction_t trans;
    memset(&trans, 0, sizeof(spi_transaction_t));
    if (command != 0) {
      trans.cmd = command;
    }
    if (address != 0) {
      trans.addr = address;
    }
    trans.rxlength = len * 8;
    trans.length = len * 8; // total length of data send and receive combined.
    trans.tx_buffer = tx_data;
    trans.rx_buffer = rx_data;
    trans.user = (void *)0;
    esp_err_t ret = spi_device_polling_transmit(configs->spi_device, &trans);
    if (ret == ESP_OK) {
      ESP_LOGD("SPI DATA", "Data received");
    } else {
      ESP_LOGE("SPI DATA", "Read error");
    }
    xSemaphoreGive(spimutex[configs->spi_host]);
    return ret;
  } else {
    return ESP_ERR_MUTEX_FAILED;
  }
}

void spi_master_init(spi_host_device_t spi_host_id, gpio_num_t miso_io,
                     gpio_num_t mosi_io, gpio_num_t sclk_io, int quadwp_io,
                     int quadhd_io, spi_common_dma_t dmaChannel) {

  spimutex[spi_host_id] = xSemaphoreCreateMutex();

  spi_bus_config_t bus_config = {
      .miso_io_num = miso_io,
      .mosi_io_num = mosi_io,
      .sclk_io_num = sclk_io,
      .quadwp_io_num = quadwp_io,
      .quadhd_io_num = quadwp_io,
  };

  ESP_ERROR_CHECK(spi_bus_initialize(spi_host_id, &bus_config, dmaChannel));
}

void spi_device_init(spi_master_functions *configs,
                     spi_host_device_t spi_host_id, int spi_mode,
                     uint32_t clock_speed, gpio_num_t cs_pin,
                     uint8_t command_bits, uint8_t address_bits,
                     int queue_size) {
  configs->spi_host = spi_host_id;
  configs->send = spi_send;
  configs->receive = spi_receive;
  configs->send_receive = spi_send_receive;
  spi_device_interface_config_t dev_config = {
      .command_bits = command_bits,
      .address_bits = address_bits,
      .dummy_bits = 0,
      .clock_speed_hz = clock_speed,
      .duty_cycle_pos = 128, // 50% duty cycle
      .mode = spi_mode,
      .spics_io_num = cs_pin,
      .cs_ena_posttrans =
          3, // Keep the CS low 3 cycles after transaction, to stop slave from
             // missing the last bit when CS has less propagation delay than CLK
      .queue_size = queue_size,
      .input_delay_ns = 400};

  ESP_ERROR_CHECK(
      spi_bus_add_device(spi_host_id, &dev_config, &configs->spi_device));
}