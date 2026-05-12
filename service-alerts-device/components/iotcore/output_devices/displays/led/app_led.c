
#include "app_led.h"

#include "driver/gpio.h"
#include "driver/ledc.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "freertos/queue.h"
#include "freertos/semphr.h"
#include "freertos/task.h"
#include "iotcore_events.h"
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef CONFIG_UNITTEST_ENABLE_ALL
#include "button.h"
#include "sdkconfig.h"
#endif

#if defined(DebugPrints) && defined(PRINT)
#define printd printf
#define LED_HEXDUMP ESP_LOG_BUFFER_HEXDUMP
#define LED_LOGI ESP_LOGI
#define LED_LOGW ESP_LOGW
#define LED_LOGE ESP_LOGE
#else

#define printd(...)
#define printd_HEXDUMP(...)
#define LED_LOGI(...)
#define LED_LOGW(...)
#define LED_LOGE(...)
#endif

#define LEDC_HS_TIMER LEDC_TIMER_0
#if SOC_LEDC_SUPPORT_HS_MODE
#define LEDC_HS_MODE LEDC_HIGH_SPEED_MODE /*!< LEDC high speed speed_mode */
#else
#define LEDC_HS_MODE LEDC_LOW_SPEED_MODE
#endif
#define LEDC_HS_CH0_CHANNEL LEDC_CHANNEL_0
#define LEDC_HS_CH1_CHANNEL LEDC_CHANNEL_1
#define LEDC_HS_CH2_CHANNEL LEDC_CHANNEL_2

#define duty2registore(d) (d * (1 << LEDC_TIMER_13_BIT) / 100)

static int8_t user_leds_count = 0;
typedef struct {
  uint32_t led;
  ledc_channel_t channel; /*!< LEDC channel (0 - 7) */
  SemaphoreHandle_t semphore;
  TaskHandle_t task;
  int8_t Taskstate;
} led_state_t;
led_state_t *user_leds = NULL;

static QueueHandle_t led_evt_queue = NULL;
static TaskHandle_t ledTask = NULL;

static const char *TAG = "LED";

bool BlinkingOn = false;
bool BlinkingwasOn = false;

bool device_sleep = 0;

void LED_blink(int freq, int duty, led_state_t *led) {
  int ch = 0;

  ledc_timer_config_t ledc_timer = {
      .duty_resolution = LEDC_TIMER_13_BIT, // resolution of PWM duty
      .freq_hz = freq,                      // frequency of PWM signal
      .speed_mode = LEDC_HS_MODE,           // timer mode
      .timer_num = LEDC_HS_TIMER            // timer index
  };
  // Set configuration of timer0 for high speed channels
  ledc_timer_config(&ledc_timer);

  ledc_channel_config_t ledc_channel[1] = {
      {.channel = led->channel,
       .duty = 0,
       .gpio_num = led->led,
       .speed_mode = LEDC_HS_MODE,
       .timer_sel = LEDC_HS_TIMER},
  };

  ledc_channel_config(&ledc_channel[ch]);

  LED_LOGI(TAG, " LEDC set duty = %d without fade\n", duty);
  ledc_set_duty(ledc_channel[ch].speed_mode, ledc_channel[ch].channel,
                duty2registore(duty));
  ledc_update_duty(ledc_channel[ch].speed_mode, ledc_channel[ch].channel);
}
void LED_blinkoff(led_state_t *led) { LED_blink(1, 0, led); }

void LED_blinkfor(blink_parameters_t *blink) {
  led_state_t *this_led = &user_leds[blink->lednumber];
  if (xSemaphoreTake(this_led->semphore, 1000 / portTICK_PERIOD_MS) == pdTRUE) {
    LED_LOGI(TAG, "led blinking with led number %d GPIO: %d", blink->lednumber,
             this_led->led);
    BlinkingOn = true;
    BlinkingwasOn = true;
    if (blink->freq >= 1) {
      if (device_sleep) // don not blink the LED in sleep mode
      {
      } else {

        LED_blink((int32_t)blink->freq, (int32_t)blink->duty, this_led);

        if (blink->events == NULL) {
          if (blink->ms > 60 * 1000)
            blink->ms = 60 * 1000;
          vTaskDelay(blink->ms / portTICK_PERIOD_MS);
        } else if (blink->events != NULL) {
          xEventGroupWaitBits(blink->events, blink->bits, false, false,
                              blink->ms);
          LED_LOGI(TAG, "turn off led blinking event expired");
        }
      }
      LED_blink(blink->freq, 0, this_led);
    } else {
      gpio_config_t io_conf;
      // disable interrupt
      io_conf.intr_type = GPIO_INTR_DISABLE;
      // set as output mode
      io_conf.mode = GPIO_MODE_OUTPUT;
      // bit mask of the pins that you want to set
      io_conf.pin_bit_mask = BIT64(this_led->led);
      // disable pull-down mode
      io_conf.pull_down_en = 0;
      // disable pull-up mode
      io_conf.pull_up_en = 0;
      // configure GPIO with the given settings
      gpio_config(&io_conf);

      LED_LOGI(TAG, "LED slow freq %f  - duty %f ", blink->freq,
               (float)blink->duty);
      float onTime = ((1 / blink->freq) * (float)blink->duty) / 100.0f;
      float offTime = (1 / blink->freq) - onTime;
      LED_LOGI(TAG, "LED slow onTime %f  - offTime %f ", onTime, offTime);
      int32_t blinktime = blink->ms;

      if (blink->events == NULL) {
        if (blink->ms > 60 * 1000)
          blink->ms = 60 * 1000;

        blinktime = blink->ms;
        while (blinktime > 0) {
          LED_LOGI(TAG, "remaing time %d ", blinktime);
          LED_LOGI(TAG, "on");
          if (device_sleep) // don not blink the LED in sleep mode
            gpio_set_level(this_led->led, 0);
          else
            gpio_set_level(this_led->led, 1);

          vTaskDelay((onTime * 1000) / portTICK_PERIOD_MS);
          blinktime -= (onTime * 1000);

          LED_LOGI(TAG, "off");
          gpio_set_level(this_led->led, 0);
          vTaskDelay((offTime * 1000) / portTICK_PERIOD_MS);
          blinktime -= (offTime * 1000);
        }
      } else if (blink->events != NULL) {
        while (!(blink->bits & xEventGroupGetBits(blink->events))) {
          LED_LOGI(TAG, "waiting for event");
          LED_LOGI(TAG, "on");
          if (device_sleep) // don not blink the LED in sleep mode
            gpio_set_level(this_led->led, 0);
          else
            gpio_set_level(this_led->led, 1);
          vTaskDelay((onTime * 1000) / portTICK_PERIOD_MS);
          blinktime -= (onTime * 1000);

          LED_LOGI(TAG, "off");
          gpio_set_level(this_led->led, 0);
          vTaskDelay((offTime * 1000) / portTICK_PERIOD_MS);
          blinktime -= (offTime * 1000);
        }
      }
    }
    BlinkingOn = false;
    xSemaphoreGive(this_led->semphore);
  }
}

void LED_blinkfor_vtask(void *pointer) {
  if (pointer != NULL) {
    blink_parameters_t blink;
    memcpy(&blink, pointer, sizeof(blink));
    user_leds[blink.lednumber].Taskstate++;

    LED_blinkfor(&blink);
    vTaskDelay(100 / portTICK_PERIOD_MS);
    user_leds[blink.lednumber].Taskstate--;
  }
  vTaskDelete(NULL);
}
void LED_blinkfor_task(blink_parameters_t *pointer) {
  if (pointer->lednumber < user_leds_count)
    xQueueSend(led_evt_queue, pointer, 0);
  else
    ESP_LOGE(TAG, "error wrong led number %d", pointer->lednumber);
}

static void led_task(void *arg) {
  blink_parameters_t state;
  for (;;) {
    if (xQueueReceive(led_evt_queue, &state, 4000 / portTICK_PERIOD_MS)) {
      BaseType_t ts = xTaskCreatePinnedToCore(
          LED_blinkfor_vtask, "LED_blinkfor_task", 4 * 1024, &state, 10,
          &user_leds[state.lednumber].task, 1);
      if (pdPASS != ts) {
        post_task_create_failed_event(__FILE__, __LINE__,
                                      esp_get_free_heap_size());
      }
      vTaskDelay(100 / portTICK_PERIOD_MS);
    }
  }
}
QueueHandle_t led_task_init(uint32_t *leds, int8_t numberofled) {
  int i = 0;

  if (numberofled) {
    LED_LOGI(TAG, "%d user led need to add", numberofled);
    user_leds_count = numberofled;
    user_leds = malloc(user_leds_count * sizeof(led_state_t));
    if (user_leds == NULL) {
      ESP_LOGE(TAG, "led adding failed - memory error");
      return NULL;
    }
    led_evt_queue = xQueueCreate(10, sizeof(blink_parameters_t));
    for (i = 0; i < user_leds_count; i++) {
      LED_LOGI(TAG, "%d user led adde GPIO:%d", i, leds[i]);
      user_leds[i].led = leds[i];
      user_leds[i].channel = LEDC_CHANNEL_0 + i;
      user_leds[i].semphore = xSemaphoreCreateMutex();
      user_leds[i].task = NULL;
      user_leds[i].Taskstate = 0;
    }

    BaseType_t ts = xTaskCreatePinnedToCore(led_task, "led_task", 3048, NULL, 5,
                                            &ledTask, 1);
    if (pdPASS != ts) {
      post_task_create_failed_event(__FILE__, __LINE__,
                                    esp_get_free_heap_size());
    }
    LED_LOGI(TAG, "task started ");
    return led_evt_queue;
  }
  return NULL;
}

#ifdef CONFIG_UNITTEST_ENABLE_ALL
esp_err_t unittest_leds() {
#if (CONFIG_USER_LEDS_TOTAL > 0)
  blink_parameters_t blink;

#if (CONFIG_USER_LEDS_1 >= 0)
  vTaskDelay(1000 / portTICK_PERIOD_MS);
  blink.duty = 60;
  blink.freq = 5;
  blink.ms = 1000;
  blink.events = NULL;
  blink.lednumber = 0;
  LED_blinkfor_task(&blink);
#endif

#if (CONFIG_USER_LEDS_2 >= 0)
  vTaskDelay(1000 / portTICK_PERIOD_MS);
  blink.duty = 60;
  blink.freq = 5;
  blink.ms = 1000;
  blink.events = NULL;
  blink.lednumber = 1;
  LED_blinkfor_task(&blink);
#endif

#if (CONFIG_USER_LEDS_3 >= 0)
  vTaskDelay(1000 / portTICK_PERIOD_MS);
  blink.duty = 60;
  blink.freq = 5;
  blink.ms = 1000;
  blink.events = NULL;
  blink.lednumber = 2;
  LED_blinkfor_task(&blink);
  vTaskDelay(1000 / portTICK_PERIOD_MS);
#endif

  xEventGroupClearBits(s_unittest_event_group, BUTTON_UNITTEST_BIT);
  printf("-----------Press Button If led blink in Red Green Blue colors else "
         "dont press\n");
  return unittest_button("LEDS");
#else
  return ESP_OK;
#endif
}
#endif
