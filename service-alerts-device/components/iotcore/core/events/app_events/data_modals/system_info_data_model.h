#ifndef _system_info_h_
#define _system_info_h_
#include "esp_types.h"

typedef struct battery_post {
  float voltage;
  float current;
  /* data */
} battery_post_t;

typedef struct reset_post {
  int reset_reason;
  int reset_count;
  int dc_code;
  int ec_code;
  int uptime_hours;

} reset_post_t;

#endif