#ifndef _si7006_a20_h_
#define _si7006_a20_h_

void app_i2c_si7006(void (*fun_ptr)(void), uint32_t measurement_time_sec);
int Si7006_Read(float *humidity, float *temperature);

#endif