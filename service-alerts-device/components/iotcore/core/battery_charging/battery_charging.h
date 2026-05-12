#ifndef _battery_charging_h_
#define _battery_charging_h_
#include "iotcore_events.h"
#include "sdkconfig.h"
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef enum {
  battery_noCharging,
  battery_Charging,
  battery_fullCharge,
  battery_noBattery,
} battery_charging_status_t;

void battery_charging_int(uint32_t ChargingPin, int32_t StandByPin);
void battery_charging_setCallback(void *cb_function);

#ifdef CONFIG_UNITTEST_ENABLE_ALL
#include <esp_err.h>

esp_err_t unittest_battery_charging(uint32_t ChargingPin, int32_t StandByPin);
#endif

#endif //_battery_charging_h_