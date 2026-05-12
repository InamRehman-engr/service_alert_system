#include "ws2812b_led_strip.h"

static const char *TAG = "example";

#define RMT_TX_CHANNEL RMT_CHANNEL_4

#define EXAMPLE_CHASE_SPEED_MS (10)
led_strip_handle_t *led_strip;

void ws2812b_set_color_on_index(uint8_t index, uint32_t value) {
  ESP_ERROR_CHECK(led_strip_set_pixel(led_strip, index, (value >> 16) & 0xFF,
                                      (value >> 8) & 0xFF, value & 0xFF));
  ESP_ERROR_CHECK(led_strip_refresh(led_strip));
}

void ws2812b_set_all(uint32_t value) {
  for (uint8_t index = 0; index < CONFIG_STRIP_LED_NUMBER; index++) {
    ESP_ERROR_CHECK(led_strip_set_pixel(led_strip, index, (value >> 16) & 0xFF,
                                        (value >> 8) & 0xFF, value & 0xFF));
  }
  ESP_ERROR_CHECK(led_strip_refresh(led_strip));
}