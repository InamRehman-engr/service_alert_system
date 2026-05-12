
#include "button.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "freertos/queue.h"
#include "freertos/task.h"
#include "iotcore_events.h"
#include "sdkconfig.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define TAG "BUTTON"

#define printd(format, ...) ESP_LOGI(TAG, format, ##__VA_ARGS__)
// #define printd(... )

#ifdef CONFIG_UNITTEST_ENABLE_ALL
EventGroupHandle_t s_unittest_event_group = NULL;
const int BUTTON_UNITTEST_BIT = BIT0;
#endif

static QueueHandle_t gpio_evt_queue = NULL;
#define ESP_INTR_FLAG_DEFAULT 0
static uint32_t userButtonsCount = 0;
typedef struct {
  uint32_t button;
  int StateLast;
  int State;
  int startTicks;
} button_state_t;
button_state_t *userbutton = NULL;

static void (*buttonPressedShort)(int32_t, int32_t) = NULL;
static void (*buttonPressedMedium)(int32_t, int32_t) = NULL;
static void (*buttonPressedLong)(int32_t, int32_t) = NULL;
static void (*buttonPressedStatehigh)(int32_t, int32_t) = NULL;
static void (*buttonPressedStateLow)(int32_t, int32_t) = NULL;

void buttonPressedCallbackShort(void (*cb_function)(int32_t, int32_t)) {
  buttonPressedShort = cb_function;
}

void buttonPressedCallbackMedium(void (*cb_function)(int32_t, int32_t)) {
  buttonPressedMedium = cb_function;
}

void buttonPressedCallbackLong(void (*cb_function)(int32_t, int32_t)) {
  buttonPressedLong = cb_function;
}

void buttonPressedCallbackState_high(void (*cb_function)(int32_t, int32_t)) {
  buttonPressedStatehigh = cb_function;
}

void buttonPressedCallbackState_low(void (*cb_function)(int32_t, int32_t)) {
  buttonPressedStateLow = cb_function;
}

static void IRAM_ATTR button_isr_handler(void *arg) {
  uint32_t gpio_num = (uint32_t)arg;
  xQueueSendFromISR(gpio_evt_queue, &gpio_num, NULL);
}

void buttonPressed(uint32_t btn, int32_t prssedTime) {
  if (prssedTime > 60 && prssedTime < 400) {
    if (buttonPressedShort != NULL)
      buttonPressedShort(btn, prssedTime);
  } else if (prssedTime > 3000 && prssedTime < 5000) {
    if (buttonPressedMedium != NULL)
      buttonPressedMedium(btn, prssedTime);
  } else if (prssedTime >= 5000) {
    if (buttonPressedLong != NULL)
      buttonPressedLong(btn, prssedTime);
  }
}

void buttonPressed_ext(uint32_t btn, int32_t prssedTime) {
  buttonPressed(btn, prssedTime);
}

int getButtonState(uint32_t buttonToGet) {
  for (int i = 0; i < userButtonsCount; i++) {
    /* code */
    if (userbutton[i].button == buttonToGet) {
      /* code */
      ESP_LOGI(TAG, "returrn button state = %d", userbutton[i].State);
      return userbutton[i].State;
    }
  }
  ESP_LOGE(TAG, "button not found");
  return -1;
}

static void Button_task(void *arg) {
  if (userbutton == NULL) {
    ESP_LOGE(TAG, "Button_task - memory error");
    vTaskDelete(NULL);
  }
  printd("Button_task\n\n");
  int count = 0;
  int64_t startTicks = esp_timer_get_time() / 1000;
  for (count = 0; count < userButtonsCount; count++) {
    userbutton[count].State = gpio_get_level(userbutton[count].button);
    userbutton[count].StateLast = 1;
    userbutton[count].startTicks = startTicks;
    printd("Button [%ld]state %d \n\n", userbutton[count].button,
           userbutton[count].State);
  }

  uint32_t io_num;
  for (;;) {
    if (xQueueReceive(gpio_evt_queue, &io_num, 1000 / portTICK_PERIOD_MS)) {
      for (count = 0; count < userButtonsCount; count++) {
        if (io_num == userbutton[count].button) {
          userbutton[count].State = gpio_get_level(io_num);
          // printd("GPIO[%d] intr, val: %d\n", io_num, userbutton[count].State
          // );
          if (userbutton[count].State == userbutton[count].StateLast) {
          } else {
            printd("State changed GPIO[%ld] intr, val: %d\n", io_num,
                   userbutton[count].State);
            if (userbutton[count].State == 0) {
              printd("\nState Low\n");
              if (buttonPressedStatehigh) {
                printd("\nbuttonPressedStatehigh\n");
                buttonPressedStatehigh(io_num, 0);
              }

              userbutton[count].startTicks = esp_timer_get_time() / 1000;
            } else {
              printd("\nState High\n");

              if (buttonPressedStateLow) {
                printd("\buttonPressedStateLow\n");
                buttonPressedStateLow(io_num, 0);
              }

              int duration =
                  (esp_timer_get_time() / 1000) - userbutton[count].startTicks;
              printd("\n****\n             GPIO[%ld] prssed for %d       "
                     "(%dms) \n",
                     io_num, (duration / 1000), duration);
              userbutton[count].startTicks = esp_timer_get_time() / 1000;
              buttonPressed(io_num, duration);
            }
            userbutton[count].StateLast = userbutton[count].State;
          }
          break;
        }
      }
      if (count == userButtonsCount) {
        // no button matched
        ESP_LOGE(TAG, "gpio_evt_queue io_num %ld ", io_num);
      }
    } else {
      for (count = 0; count < userButtonsCount; count++) {
        userbutton[count].State = gpio_get_level(userbutton[count].button);
        // printd("buttonState: %d\n", userbutton[count].State);

        // Unused
        /// TODO: find out what this was for and fix this later
        // if (userbutton[count].State == 0)
        // {
        //     int duration = (esp_timer_get_time() / 1000) -
        //     userbutton[count].startTicks;
        //     // printd("     button [%ld] prssed for %d       (%dms) \n",
        //     userbutton[count].button, duration, duration);
        // }
        // else
        // {
        // }
      }
    }
  }
}

void button_int(uint32_t *buttons, int32_t numberOfButtons,
                GPIO_CONFIG config[]) {
  gpio_config_t io_conf;
  esp_err_t err = -1;
  int i = 0;

  if (numberOfButtons) {
    ESP_LOGI(TAG, "%ld user button need to add", numberOfButtons);
    userButtonsCount = numberOfButtons;
    userbutton = malloc(userButtonsCount * sizeof(button_state_t));

    if (userbutton) {
    } else {
      ESP_LOGE(TAG, "button adding failed - memory error");
      return;
    }
    // create a queue to handle gpio event from isr
    gpio_evt_queue = xQueueCreate(10, sizeof(uint32_t));

    for (i = 0; i < numberOfButtons; i++) {
      userbutton[i].button = buttons[i];

      // interrupt of rising edge
      io_conf.intr_type = GPIO_INTR_ANYEDGE;
      // bit mask of the pins, use GPIO4/5 here
      io_conf.pin_bit_mask = BIT64(userbutton[i].button);
      // set as input mode
      io_conf.mode = GPIO_MODE_INPUT;
      switch (config[i]) {
      case PULL_UP:
        // disable pull-down mode
        io_conf.pull_down_en = 0;
        // enable pull-up mode
        io_conf.pull_up_en = 1;
        break;
      case PULL_DOWN:
        // disable pull-down mode
        io_conf.pull_down_en = 1;
        // enable pull-up mode
        io_conf.pull_up_en = 0;
        break;
      case PULL_NONE:
        // disable pull-down mode
        io_conf.pull_down_en = 0;
        // enable pull-up mode
        io_conf.pull_up_en = 0;
        break;
      default:
        // disable pull-down mode
        io_conf.pull_down_en = 0;
        // enable pull-up mode
        io_conf.pull_up_en = 0;
        break;
      }
      gpio_config(&io_conf);
    }

    // start gpio task
    BaseType_t ts = xTaskCreatePinnedToCore(Button_task, "Button_task", 4048,
                                            NULL, 5, NULL, 1);
    if (pdPASS != ts) {
      post_task_create_failed_event(__FILE__, __LINE__,
                                    esp_get_free_heap_size());
    }

    // install gpio isr service
    err = gpio_install_isr_service(ESP_INTR_FLAG_DEFAULT);
    if (err != ESP_OK)
      post_iotcore_error_event(GENERAL_ESP_ERROR_CODE, &err, sizeof(int));

    for (i = 0; i < numberOfButtons; i++) {
      // hook isr handler for specific gpio pin
      err = gpio_isr_handler_add(userbutton[i].button, button_isr_handler,
                                 (void *)userbutton[i].button);
      if (err != ESP_OK)
        post_iotcore_error_event(GENERAL_ESP_ERROR_CODE, &err, sizeof(int));

      ESP_LOGI(TAG, "%d: user button %ld added", i, userbutton[i].button);

      xQueueSend(gpio_evt_queue, &userbutton[i].button, 0);
    }
  }
}

#ifdef CONFIG_UNITTEST_ENABLE_ALL
esp_err_t unittest_button(char *unit) {
  EventBits_t uxBits;
  printf("-------------------PRESS BUTTON FOR A SEC\nwaiting . . .\n");
  uxBits = xEventGroupWaitBits(s_unittest_event_group, BUTTON_UNITTEST_BIT,
                               pdTRUE, pdFALSE, pdMS_TO_TICKS(10000));
  if (uxBits & BUTTON_UNITTEST_BIT) {
    ESP_LOGW("UNITTEST", "%s OK", unit);
    // xEventGroupClearBits(s_unittest_event_group, BUTTON_UNITTEST_BIT);
    return ESP_OK;
  } else {
    ESP_LOGE("UNITTEST", "%s ISSUE", unit);
    return ESP_FAIL;
  }
}
#endif