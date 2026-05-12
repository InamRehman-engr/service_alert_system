#ifndef HARDWARE_H
#define HARDWARE_H

#include <stdint.h>

extern const int SAMPLE_RATE;

void beep(float freq = 1000.0f, float duration_sec = 0.1f, float vol = 0.5f);
void vibrate_haptic(uint16_t ms);
int get_battery_percent();
void check_and_beep_low_battery();
void set_charge_current_350mA();

#endif  // HARDWARE_H