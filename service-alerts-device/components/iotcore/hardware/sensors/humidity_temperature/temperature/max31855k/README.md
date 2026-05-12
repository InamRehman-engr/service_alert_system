# MAX31855K updated last 5/3/24


Usage of library
```
#include <stdio.h> 
#include "max31855k.h"

max31855k_handle_t handle1, handle2;

void app_main(void)
{
    esp_err_t check;
    spi_master_init(SPI3_HOST, GPIO_NUM_17, -1, GPIO_NUM_18, -1, -1, SPI_DMA_DISABLED);
    vTaskDelay(pdMS_TO_TICKS(1000));
    spi_master_init(SPI2_HOST, GPIO_NUM_25, -1, GPIO_NUM_26, -1, -1, SPI_DMA_DISABLED);
    spi_device_init(&handle1.max31855k, SPI3_HOST, 0, 100000, GPIO_NUM_23, 0, 0, 1);
    spi_device_init(&handle2.max31855k, SPI2_HOST, 0, 10000, GPIO_NUM_21, 0, 0, 1);
    while (true)
    {
        max31855k_getData(&handle1);
        ESP_LOGE("1 -> FAULT: ", "%d", handle1.faultBit);
        ESP_LOGE("1 -> VCC SHORT: ", "%d", handle1.shortCircuitHighBit);
        ESP_LOGE("1 -> GND SHORT: ", "%d", handle1.shortCircuitLowBit);
        ESP_LOGE("1 -> OPEN CIRCUIT: ", "%d", handle1.openCircuitBit);
        ESP_LOGE("1 -> JUNCTION TEMPERATURE: ", "%f", handle1.junctionTemperature);
        ESP_LOGE("1 -> THERMOCOUPLE TEMPERATURE: ", "%f", handle1.thermocoupleTemperature);
        ESP_LOGE("1 -> CELSIUS TEMPERATURE: ", "%f", handle1.temperature);

        max31855k_getData(&handle2);
        ESP_LOGI("2 -> FAULT: ", "%d", handle2.faultBit);
        ESP_LOGI("2 -> VCC SHORT: ", "%d", handle2.shortCircuitHighBit);
        ESP_LOGI("2 -> GND SHORT: ", "%d", handle2.shortCircuitLowBit);
        ESP_LOGI("2 -> OPEN CIRCUIT: ", "%d", handle2.openCircuitBit);
        ESP_LOGI("2 -> JUNCTION TEMPERATURE: ", "%f", handle2.junctionTemperature);
        ESP_LOGI("2 -> THERMOCOUPLE TEMPERATURE: ", "%f", handle2.thermocoupleTemperature);
        ESP_LOGI("2 -> CELSIUS TEMPERATURE: ", "%f", handle2.temperature);

        vTaskDelay(pdMS_TO_TICKS(3000));
    }
}

```