#ifndef _MiCS_5524_h_
#define _MiCS_5524_h_

#include "driver/adc.h"
#include "esp_adc_cal.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/task.h"
#include "sdkconfig.h"
#include <stdio.h>
#include <stdlib.h>
extern QueueHandle_t HandleToQueue;

QueueHandle_t GetReadingFromMiCSTask(int, TaskHandle_t *);
#endif