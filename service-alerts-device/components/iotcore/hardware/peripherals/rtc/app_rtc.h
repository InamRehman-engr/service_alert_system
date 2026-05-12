#ifndef _APP_RTC_H_
#define _APP_RTC_H_

#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "freertos/task.h"
#include <stdint.h>
#include <stdio.h>
#include <sys/time.h>
#include <time.h>
extern int timezoneMinutes;
extern EventGroupHandle_t rtc_event_group;
extern const int TIME_SYNC_BIT;
typedef struct {
  bool status;
  time_t remaining_time;
} timeout_info;

void app_rtc(void *ptr);
void SetTimeZone(int32_t timezoneMinutes);
void time_local(time_t *seconds);
void gettimeofday_local(struct timeval *tv);
int32_t time_currentmonth(time_t seconds);
void time_nextmonth_seconds(time_t current_seconds, time_t *next_seconds);
uint64_t time_ms(uint64_t *milliseconds);
void time_ms_local(uint64_t *milliseconds);
// extern void app_rtc();
void app_rtc_cb_func_set(void (*cb_func)(void));
esp_err_t unittest_rtc();
void get_sim868_rtc();
timeout_info check_for_timeout(time_t start_time, int time_threshold);
#endif //_APP_RTC_H_