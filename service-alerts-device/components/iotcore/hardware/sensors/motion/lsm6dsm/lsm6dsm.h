#ifndef LSM6DSM_H
#define LSM6DSM_H
#include "freertos/queue.h"
#include "freertos/task.h"
#include "lsm6dsm_reg.h"
#include "sdkconfig.h"
#include <freertos/FreeRTOS.h>
// structure for storing sensor measurements
struct LSM6DSO_SensorData {
  double LinearAcceleration;
  double Pitch;
  double Roll;
};

QueueHandle_t LSM6DSO_GetSensorReadingTask(int SizeOfQueue,
                                           TaskHandle_t *TaskHandle);
#endif