
#ifndef _MC106_h_
#define _MC106_h_

#include "driver/adc.h"
#include "esp_adc_cal.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "sdkconfig.h"
#include <stdio.h>
#include <stdlib.h>

void adc_calibration_init(void);
esp_err_t ADC_init(void);
void get_MC106(void);
#endif