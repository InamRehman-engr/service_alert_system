# MAX31855K updated last 5/3/24


Usage of library
```
//I2C Usage
#include <stdio.h>
#include "max31855k.h"
#include "bmp280.h"

bmp280_config_t bmp280_config = {
    .interface = BMP280_I2C,
    .mode = BMP280_MODE_NORMAL,
    .address = BMP280_SDO_LOW_ADDRESS,
    .tempSampling = BMP280_SAMPLING_X16,
    .pressSampling = BMP280_SAMPLING_X16,
    .filter = BMP280_FILTER_OFF,
    .duration = BMP280_STANDBY_MS_1};

void app_main(void)
{
    uint32_t heap_change = esp_get_free_heap_size();
    float temperature, pressure, altitude;
    i2c_device_t dev = {.port = I2C_NUM_0, .i2c_mutex = NULL};
    i2c_functions i2cfcns = {.device = &dev};
    i2c_init(I2C_MODE_MASTER, GPIO_NUM_21, GPIO_NUM_22, true, true, 50000, &i2cfcns);
    bmp280_handle_t handle = {.bmp280_interface.i2c = &i2cfcns};

    bmp280_init(&handle, &bmp280_config);

    while (true)
    {
        
        handle.getData(&handle);
        ESP_LOGE("TEMPERATURE: ", "%f", handle.temperature);
        ESP_LOGE("PRESSURE: ", "%f", handle.pressure);
        heap_change = heap_change - esp_get_free_heap_size();
        ESP_LOGW("MEM", "free heap: %ld", esp_get_free_heap_size());
        ESP_LOGW("MEM", "heap change: %ld", heap_change);
        heap_change = esp_get_free_heap_size();

        vTaskDelay(pdMS_TO_TICKS(500));
    }
}

//IMPORTANT NOTE: SPI is not yet supported. Support will be added after correcting implementation of SPI Master Driver

//SPI Usage
#include <stdio.h>
#include "max31855k.h"
#include "bmp280.h"

bmp280_handle_t handle;

bmp280_config_t bmp280_config = {
    .interface = BMP280_SPI,
    .mode = BMP280_MODE_NORMAL,
    .address = BMP280_SDO_LOW_ADDRESS,
    .tempSampling = BMP280_SAMPLING_X16,
    .pressSampling = BMP280_SAMPLING_X16,
    .filter = BMP280_FILTER_OFF,
    .duration = BMP280_STANDBY_MS_1};

void app_main(void)
{
    uint32_t heap_change = esp_get_free_heap_size();
    float temperature, pressure, altitude;
    vTaskDelay(pdMS_TO_TICKS(2000));

    
    spi_master_init(SPI2_HOST, GPIO_NUM_25, -1, GPIO_NUM_26, -1, -1, SPI_DMA_DISABLED);
    spi_device_init(&handle.bmp280_spi, SPI2_HOST, 0, 100000, GPIO_NUM_23, 0, 0, 1);
    bmp280_init(&handle, &bmp280_config);

    while (true)
    {
        temperature = handle.getTemperature(&handle);
        pressure = handle.getPressure(&handle);
        altitude = handle.getAltitude(&handle, 1016);

        ESP_LOGE("1 -> TEMPERATURE: ", "%f", temperature);
        ESP_LOGE("1 -> PRESSURE: ", "%f", pressure);
        ESP_LOGE("1 -> ALTITUDE: ", "%f", altitude);

        heap_change = heap_change - esp_get_free_heap_size();
        ESP_LOGW("MEM", "free heap: %ld", esp_get_free_heap_size());
        ESP_LOGW("MEM", "heap change: %ld", heap_change);
        heap_change = esp_get_free_heap_size();

        vTaskDelay(pdMS_TO_TICKS(500));
    }
}

```