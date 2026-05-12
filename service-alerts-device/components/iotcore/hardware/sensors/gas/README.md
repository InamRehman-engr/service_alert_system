# Gas Sensors

## MiCS-5524

## Prerequisites

Following are the hardware and software pre-req for using the library:  

### Hardware

`Module` MiCS-5524 Module  
`MCU` ESP32

|ESP32|MiCS-5524 Module|
|--|--|
|5V|5V|
|GND|GND|
|ADC pin|AO|

### Software Components

`ESP-IDF Version` The code has been tested on ESP-IDF version 5.0. It is recommended to use IDF ver 5.0 when using the library.  
`MiCS_5524.h` The MiCS_5524.h file contains the library functions for using the MiCS-5524 sensor.  

## Usage  

The following example demonstrates the usage of the MiCS-5524 library in an ESP32 project.  

### menuconfig

Before running the code, use `menuconfig` to configure the ADC related parameters first.

### Code Example

```c
#include <stdio.h>
#include "MiCS_5524.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"

void app_main(void)
{
    TaskHandle_t tskHandle;
    QueueHandle_t qHandle = GetReadingFromMiCSTask(20, &tskHandle);
    int pvBuffer = 0;
    for(int i=0; i<10; i++)
    {
        xQueueReceive(qHandle, &pvBuffer, portMAX_DELAY);
        printf("\n%d\n", pvBuffer);
    }    
    vTaskDelete(tskHandle);
    printf("\nTask Deleted\n");

    xQueueReceive(qHandle, &pvBuffer, portMAX_DELAY);
    printf("\n%d\n", pvBuffer);
    vQueueDelete(qHandle);
}
```

### GetReadingFromMiCSTask()

The `GetReadingFromMiCSTask()` function is responsible for starting a FreeRTOS task that periodically retrieves sensor readings and pushes them into a queue. Here's a breakdown of the function:
