
#ifndef _shtc3_h_
#define _shtc3_h_

#include <stdint.h>

int shtc3_Read(float *humidity, float *temperature);
void app_i2c_shtc3(void (*fun_ptr)(void), uint32_t measurement_time_sec);
#if defined(CONFIG_UNITTEST_ENABLE_ALL) ||                                     \
    defined(CONFIG_AUTMA_DEVICE_TYPE_SIMULATOR)
esp_err_t unittest_shtc3(float *Temperature, float *Humidity);
#endif

#endif
