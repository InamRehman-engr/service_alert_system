#ifndef __ws2812b_led_strip__
#define __ws2812b_led_strip__
#include "driver/rmt.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "led_strip.h"
#include "sdkconfig.h"

typedef enum {
  RED = 0xFF0000,
  GREEN = 0x00FF00,
  BLUE = 0x0000FF,
  ORANGE = 0xFFA500,
  WHITE = 0xFFFFFF,
  BLACK = 0x000000,
  YELLOW = 0xFFFF00,
  CYAN = 0x00FFFF,
  MAGENTA = 0xFF00FF,
  PINK = 0xFFC0CB,
  PURPLE = 0x800080,
  BROWN = 0xA52A2A
} LED_colors_t;

void ws2812b_set_color_on_index(uint8_t index, uint32_t value);
void ws2812b_set_all(uint32_t value);
#endif