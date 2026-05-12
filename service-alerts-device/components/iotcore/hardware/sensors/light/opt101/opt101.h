#ifndef _opt_h_
#define _opt_h_

#include "sdkconfig.h"
#include "stdbool.h"
#include "stdlib.h"
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "driver/adc.h"
#include "driver/gpio.h"

#include "esp_adc_cal.h"
#include "esp_err.h"
#include "esp_log.h"

esp_err_t adc_init();
uint32_t get_adc();
#endif //_opt_h_