

#ifndef _app_led_h_
#define _app_led_h_

#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "freertos/queue.h"
#include "freertos/task.h"

typedef struct {
  float freq;
  float duty;
  int ms;
  EventGroupHandle_t events;
  EventBits_t bits;
  uint8_t lednumber;
} blink_parameters_t;

extern bool BlinkingOn;
extern bool BlinkingwasOn;

void LED_blinkfor_task(blink_parameters_t *pointer);
QueueHandle_t led_task_init(uint32_t *leds, int8_t numberofled);

#ifdef CONFIG_UNITTEST_ENABLE_ALL
esp_err_t unittest_leds();
#endif

#endif