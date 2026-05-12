#ifndef __SBS_BATTERY_H__
#define __SBS_BATTERY_H__

#include "esp_err.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "i2c-dev.h"
#include "sdkconfig.h"
#define SBS_BATTERY_ADDRESS CONFIG_BQ_ADDRESS
#define SBS_DATA(_addr, _min_value, _max_value)                                \
  { .addr = _addr, .min_value = _min_value, .max_value = _max_value }

enum {
  BAT_AT_RATE_TIME_TO_FULL,
  BAT_AT_RATE_TIME_TO_EMPTY,
  BAT_TEMPERATURE,
  BAT_VOLTAGE,
  BAT_CURRENT,
  BAT_AVERAGE_CURRENT,
  BAT_RELATIVE_STATE_OF_CHARGE,
  BAT_ABSOLUTE_STATE_OF_CHARGE,
  BAT_REMAINING_CAPACITY,
  BAT_FULL_CHARGE_CAPACITY,
  BAT_RUN_TIME_TO_EMPTY,
  BAT_AVERAGE_TIME_TO_EMPTY,
  BAT_AVERAGE_TIME_TO_FULL,
  BAT_CHARGING_CURRENT,
  BAT_CHARGING_VOLTAGE,
  BAT_BATTERY_STATUS,
  BAT_CYCLE_COUNT,
  BAT_DESIGN_CAPACITY,
  BAT_DESIGN_VOLTAGE,
  BAT_MANUFACTURER_DATE,
  BAT_SERIAL_NUMBER,
  BAT_MANUFACTURER_NAME,
  BAT_DEVICE_NAME,
  BAT_DEVICE_CHEMISTRY,
  BAT_MANUFACTURER_DATA,
  BAT_CELL_VOLTAGE_4,
  BAT_CELL_VOLTAGE_3,
  BAT_CELL_VOLTAGE_2,
  BAT_CELL_VOLTAGE_1,
  BAT_STATE_OF_HEALTH,
  BAT_TURBO_POWER,
  BAT_TURBO_CURRENT,
};
typedef struct {
  uint16_t at_rate_time_to_full;
  uint16_t at_rate_time_to_empty;
  uint16_t temperature;
  uint16_t voltage;
  int16_t current;
  int16_t average_current;
  uint8_t relative_state_of_charge;
  uint8_t absolute_state_of_charge;
  uint16_t remaining_capacity;
  uint16_t full_charge_capacity;
  uint16_t run_time_to_empty;
  uint16_t average_time_to_empty;
  uint16_t average_time_to_full;
  uint16_t charging_current;
  uint16_t charging_voltage;
  uint16_t battery_status;
  uint16_t cycle_count;
  uint16_t design_capacity;
  uint16_t design_voltage;
  uint16_t manufacturer_date;
  uint16_t serial_number;
  char manufacturer_name[15];
  char device_name[15];
  char device_chemistry[15];
  uint16_t voltage_cell_1;
  uint16_t voltage_cell_2;
  uint16_t voltage_cell_3;
  uint16_t voltage_cell_4;
  uint8_t state_of_health;
  uint16_t turbo_power;
  uint16_t turbo_current;
} battery_data;
esp_err_t sbs_battery_init(battery_data *data,
                           SemaphoreHandle_t *read_write_mutex);

#endif