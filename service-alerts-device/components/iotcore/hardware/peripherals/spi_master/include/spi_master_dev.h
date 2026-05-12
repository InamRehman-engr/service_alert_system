#ifndef SPI_MASTER_DEV_H
#define SPI_MASTER_DEV_H

#include "driver/gpio.h"
#include "driver/spi_master.h"
#include "esp_err.h"
#include "esp_system.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "freertos/task.h"
#include "string.h"
#define ESP_ERR_MUTEX_FAILED (255)

struct spi_master_functions {
  spi_device_handle_t spi_device;
  spi_host_device_t spi_host;
  esp_err_t (*send)(struct spi_master_functions *configs, uint16_t command,
                    uint64_t address, uint8_t *data, size_t len);
  esp_err_t (*receive)(struct spi_master_functions *configs, uint16_t command,
                       uint64_t address, uint8_t *data, size_t len);
  esp_err_t (*send_receive)(struct spi_master_functions *configs,
                            uint16_t command, uint64_t address,
                            uint8_t *tx_data, uint8_t *rx_data, size_t len);
};

typedef struct spi_master_functions spi_master_functions;

void spi_master_init(spi_host_device_t spi_host_id, gpio_num_t miso_io,
                     gpio_num_t mosi_io, gpio_num_t sclk_io, int quadwp_io,
                     int quadhd_io, spi_common_dma_t dmaChannel);

void spi_device_init(spi_master_functions *configs,
                     spi_host_device_t spi_host_id, int spi_mode,
                     uint32_t clock_speed, gpio_num_t cs_pin,
                     uint8_t command_bits, uint8_t address_bits,
                     int queue_size);

#endif