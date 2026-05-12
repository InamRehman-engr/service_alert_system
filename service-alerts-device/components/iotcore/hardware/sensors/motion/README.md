# Motion Sensors

## LSM6DSO README

## Prerequisites

Following are the hardware and software pre-req for using this library:  

### Hardware

`Module` STEVAL$MKI197V1  
`MCU` ESP32

|ESP32|LSM6DSO Module|
|--|--|
|3V3|VDD|
|GND|GND|
|IO26|SDA|
|IO25 |SCL|
`lsm6dsm_reg.h` This file provides the driver code for the LSM6DSO sensor, which is provided by ST.  
`lsm6dsm.h` The lsm6dsm.h file contains the API functions for using the LSM6DSO sensor.  
Both **lsm6dsm_reg.h** and **lsm6dsm.h** are necessary for successful code compilation. Ensure both files are included.  

## Usage  

The following example demonstrates the usage of the LSM6DSO library in an ESP32 project.  

```C
#include <string.h>
#include <stdio.h>
#include "driver/i2c.h"
#include "esp_log.h"
#include <math.h>
#include "lsm6dsm.h"
#include "lsm6dsm_reg.h"
#include "sdkconfig.h"
int main()
{
  int SizeOfQueue = 10;
  TaskHandle_t tskHandle;
  QueueHandle_t qHandle = LSM6DSO_GetSensorReadingTask(SizeOfQueue, &tskHandle);
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
