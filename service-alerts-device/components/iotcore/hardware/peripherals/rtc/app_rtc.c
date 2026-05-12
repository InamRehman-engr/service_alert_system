
#include "app_rtc.h"
#include "connectivity.h"
#include "esp_attr.h"
#include "esp_log.h"
#include "esp_sleep.h"
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "freertos/queue.h"
#include "freertos/semphr.h"
#include "freertos/task.h"
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <time.h>
#ifdef GSM_MODEM_AVAILABLE
#include "sim868.h"
#endif
#include "esp_sntp.h"
#include "lwip/err.h"
#include "nvs_flash.h"
RTC_DATA_ATTR static int boot_count = 0;
#define WAIT_FOR_RTC_SYNC 24 * 60 * 60

const static char *TAG = "RTC";
int timezoneMinutes = CONFIG_TIMEZONE_OFFSET;
static void (*app_rtc_cb_func)(void) = NULL;
void app_rtc_cb_func_set(void (*cb_func)(void)) { app_rtc_cb_func = cb_func; }
EventGroupHandle_t rtc_event_group = NULL;
const int TIME_SYNC_BIT = BIT0;
static void obtain_time(void);
static void initialize_sntp(void);

void time_local(time_t *seconds) {
  time(seconds);
  *seconds += (timezoneMinutes * 60);
}
void gettimeofday_local(struct timeval *tv) {
  gettimeofday(tv, NULL);
  tv->tv_sec += (timezoneMinutes * 60);
}
uint64_t time_ms(uint64_t *milliseconds) {
  uint64_t ms = 0;
  struct timeval tv_now;
  gettimeofday(&tv_now, NULL);
  ms = ((double)tv_now.tv_sec * 1000) + ((double)tv_now.tv_usec / 1000);
  if (milliseconds)
    *milliseconds = ms;
  return ms;
}
void time_ms_local(uint64_t *milliseconds) {
  time_ms(milliseconds);
  *milliseconds += (timezoneMinutes * 60 * 1000);
}

int32_t time_currentmonth(time_t seconds) {
  struct tm timem;
  gmtime_r(&seconds, &timem);
  return timem.tm_mon;
}

void time_nextmonth_seconds(time_t current_seconds, time_t *next_seconds) {
  struct tm timem;
  gmtime_r(&current_seconds, &timem);
  if (timem.tm_mon < 11)
    timem.tm_mon++;
  else {
    timem.tm_mon = 0;
    timem.tm_year++;
  }
  timem.tm_hour = 0;
  timem.tm_mday = 1;
  timem.tm_min = 0;
  timem.tm_sec = 0;
  time_t ticks_of_next_month = mktime(&timem);
  *next_seconds = ticks_of_next_month;
}

void SetTimeZone(int32_t timezoneMinutes) {
  // Set timezone  print local time
  char timezoneString[30] = "GMT-5";
  int timezoneMinus = 0;
  if (timezoneMinutes == 0) {
    sprintf(timezoneString, "%s", "GMT0");
  } else if (timezoneMinutes > 0) {
    if (timezoneMinutes % 60 == 0) {
      sprintf(timezoneString, "<+%02ld>-%ld", timezoneMinutes / 60,
              timezoneMinutes / 60);
    } else {
      sprintf(timezoneString, "<+%02ld%02ld>-%ld:%02ld", timezoneMinutes / 60,
              timezoneMinutes % 60, timezoneMinutes / 60, timezoneMinutes % 60);
    }
  } else if (timezoneMinutes < 0) {
    timezoneMinus = timezoneMinutes * -1;
    if (timezoneMinus % 60 == 0) {
      sprintf(timezoneString, "<-%02d>+%d", timezoneMinus / 60,
              timezoneMinus / 60);
    } else {
      sprintf(timezoneString, "<-%02d%02d>+%d:%02d", timezoneMinus / 60,
              timezoneMinus % 60, timezoneMinus / 60, timezoneMinus % 60);
    }
  }
  ESP_LOGI(TAG, "timezone %s \n", timezoneString);
  setenv("TZ", timezoneString, 1);
  tzset();
}

void time_sync_notification_cb(struct timeval *tv) {
  struct tm timeinfo = {0};
  char strftime_buf[64];
  time_t seconds = tv->tv_sec + (timezoneMinutes * 60);
  gmtime_r(&seconds, &timeinfo);
  // localtime_r(&tv->tv_sec, &timeinfo);
  strftime(strftime_buf, sizeof(strftime_buf), "%c", &timeinfo);
  xEventGroupSetBits(rtc_event_group, TIME_SYNC_BIT);

  ESP_LOGW(TAG, "\nNotification of a time synchronization event\n");
  ESP_LOGI(TAG, "The current date and time is: %s (%lli.%li)", strftime_buf,
           tv->tv_sec, tv->tv_usec);

  if (app_rtc_cb_func)
    app_rtc_cb_func();
}

static void initialize_sntp(void) {
  ESP_LOGI(TAG, "Initializing SNTP");
  esp_sntp_setoperatingmode(SNTP_OPMODE_POLL);
  esp_sntp_setservername(0, "time.windows.com");
  esp_sntp_setservername(1, "time.google.com");
  sntp_set_time_sync_notification_cb(time_sync_notification_cb);

  esp_sntp_init();
}
static void obtain_time(void) {
  waitDeviceHasInternet();
  /// TODO: NetworkConnectWait(portMAX_DELAY);
  ESP_LOGW(TAG, "obtain_time ....");
  initialize_sntp();

  // wait for time to be set
  time_t now = 0;
  struct tm timeinfo = {0};
  int retry = 0;
  const int retry_count = 100;
  while (timeinfo.tm_year < (2019 - 1900) && ++retry < retry_count) {
    ESP_LOGI(TAG, "Waiting for system time to be set... (%d/%d)", retry,
             retry_count);
    vTaskDelay(2000 / portTICK_PERIOD_MS);
    time_local(&now);
    gmtime_r(&now, &timeinfo);
  }

  // sntp_stop();
}
void app_rtc(void *ptr) {
  waitDeviceHasInternet();
#ifndef CONFIG_ENABLE_SIM868_RTC
  ++boot_count;
  ESP_LOGI(TAG, "Boot count: %d", boot_count);

  time_t now;
  struct tm timeinfo;
  rtc_event_group = xEventGroupCreate();
  SetTimeZone(timezoneMinutes); // default timezone pakistan
  time_local(&now);
  gmtime_r(&now, &timeinfo);
  // Is time set? If not, tm_year will be (1970 - 1900).
  if (timeinfo.tm_year < (2019 - 1900)) {
    xEventGroupClearBits(rtc_event_group, TIME_SYNC_BIT);
    ESP_LOGI(
        TAG,
        "Time is not set yet. Connecting to WiFi and getting time over NTP.");
    // update 'now' variable with current time
    time(&now);
  }
  char strftime_buf[64];
  SetTimeZone(timezoneMinutes); // default timezone pakistan
  time_local(&now);
  gmtime_r(&now, &timeinfo);
  // localtime_r(&now, &timeinfo);
  strftime(strftime_buf, sizeof(strftime_buf), "%c", &timeinfo);
  ESP_LOGI(TAG, "The current date and time is: %s", strftime_buf);
  obtain_time();
#else
  struct timeval tv;
  time_t now = 0;
  time_t NextTimeToSyncRTC = 0;
  struct tm timem;
  sim868_local_time_t timeinfo = {'\0'};
  if (dce)
    dce->read_local_time(dce, &timeinfo);
  const int retry_count = 100;
  int retry = 0;
  esp_err_t rtc_init = ESP_FAIL;
  while (1) {
    time(&now);
    if (++retry < retry_count && dce && now > NextTimeToSyncRTC) {
      if (rtc_init == ESP_OK) {
        if (dce->read_local_time(dce, &timeinfo) == ESP_OK &&
            timeinfo.tm_year >= 21)
          timeinfo.tm_year += 2000;

        printf("timeinfo.tm_year = %d\n ", timeinfo.tm_year);
        if (timeinfo.tm_year < (2021 - 1900)) {
          ESP_LOGI(TAG, "Waiting for system time to be set... (%d/%d)", retry,
                   retry_count);
          vTaskDelay(2000 / portTICK_PERIOD_MS);
        } else {
          timem.tm_year = timeinfo.tm_year - 1900;
          timem.tm_mon = timeinfo.tm_mon - 1;
          timem.tm_hour = timeinfo.tm_hour;
          timem.tm_mday = timeinfo.tm_mday;
          timem.tm_min = timeinfo.tm_min;
          timem.tm_sec = timeinfo.tm_sec;
          printf("timem.tm_mon = %d\n ", timem.tm_mon);
          printf("timem.tm_hour = %d\n ", timem.tm_hour);
          printf("timem.tm_mday = %d\n ", timem.tm_mday);
          printf("timem.tm_min = %d\n ", timem.tm_min);
          printf("timem.tm_sec = %d\n ", timem.tm_sec);
          time_t timestampsec = mktime(&timem);
          tv.tv_sec = timestampsec;
          tv.tv_usec = 0;
          settimeofday(&tv, NULL);
          time(&now);
          retry = 0;
          NextTimeToSyncRTC = WAIT_FOR_RTC_SYNC + now;
        }
      } else {
        rtc_init = dce->init_rtc_profile(dce);
      }
    }
    vTaskDelay(pdMS_TO_TICKS(10000));
  }
#endif
  vTaskDelete(NULL);
}

esp_err_t unittest_rtc() {
  EventBits_t bits = xEventGroupWaitBits(rtc_event_group, TIME_SYNC_BIT, pdTRUE,
                                         pdFALSE, pdMS_TO_TICKS(180000));
  if (bits & TIME_SYNC_BIT) {
    ESP_LOGW("UNITTEST", "RTC TIME OK");
    return ESP_OK;
  } else {
    ESP_LOGE("UNITTEST", "RTC TIME ISSUE");
    return ESP_FAIL;
  }
}

timeout_info check_for_timeout(time_t start_time, int time_threshold) {
  time_t t_now;
  timeout_info timeout_data;
  timeout_data.status = (time(&t_now) - start_time) > time_threshold;
  timeout_data.remaining_time =
      timeout_data.status ? 0 : time_threshold - (time(&t_now) - start_time);
  return timeout_data;
}