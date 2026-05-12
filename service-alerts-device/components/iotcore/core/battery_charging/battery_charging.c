
#include "battery_charging.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "freertos/queue.h"
#include "freertos/task.h"
#include "sdkconfig.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define TAG "BATT"

#ifdef CONFIG_UNITTEST_ENABLE_ALL
extern EventGroupHandle_t s_unittest_event_group;
const int BATT_CHG_UNITTEST_BIT = BIT1;
#endif

#define printd(format, ...) ESP_LOGI(TAG, format, ##__VA_ARGS__)
// #define printd(... )

static QueueHandle_t gpio_evt_queue = NULL;
#define ESP_INTR_FLAG_DEFAULT 0
static uint32_t userButtonsCount = 0;
TaskHandle_t x_Batt_Chg_Task_Handle = NULL;
typedef struct {
  uint32_t button;
  int StateLast;
  int State;
  int startTicks;
} button_state_t;
button_state_t *batteryGPIOs = NULL;

uint32_t battery_charging_status_pin = GPIO_NUM_NC;
uint32_t battery_charging_stdby_pin = GPIO_NUM_NC;
battery_charging_status_t battery_charging_status = battery_noCharging;

static void (*battery_charging_status_cb)(battery_charging_status_t) = NULL;

void battery_charging_setCallback(void *cb_function) {
  battery_charging_status_cb = cb_function;
}

static void IRAM_ATTR battery_charging_isr_handler(void *arg) {
  uint32_t gpio_num = (uint32_t)arg;
  xQueueSendFromISR(gpio_evt_queue, &gpio_num, NULL);
}

void battery_charging_status_change(battery_charging_status_t status) {
  if (battery_charging_status_cb)
    battery_charging_status_cb(status);
}

static void battery_charging_task(void *arg) {

  if (batteryGPIOs == NULL) {
    ESP_LOGE(TAG, "battery_charging_task - memory error");
    vTaskDelete(NULL);
  }
  printd("battery_charging_task\n\n");
  int count = 0;
  int64_t startTicks = esp_timer_get_time() / 1000;

  for (count = 0; count < userButtonsCount; count++) {
    batteryGPIOs[count].State = gpio_get_level(batteryGPIOs[count].button);
    batteryGPIOs[count].StateLast = 1;
    batteryGPIOs[count].startTicks = startTicks;
    printd("Button [%ld]state %d \n\n", batteryGPIOs[count].button,
           batteryGPIOs[count].State);
  }
  int chargingState_STDBY_last = -1;
  int chargingState_status_last = -1;
  int chargingState_STDBY = -1;
  if (battery_charging_stdby_pin != GPIO_NUM_NC)
    chargingState_STDBY = gpio_get_level(battery_charging_stdby_pin);
  int chargingState_status = -1;
  if (battery_charging_status_pin != GPIO_NUM_NC)
    chargingState_status = gpio_get_level(battery_charging_status_pin);
  uint32_t io_num;
  battery_charging_status_t status = battery_charging_status;

  for (;;) {
    if (xQueueReceive(gpio_evt_queue, &io_num, pdMS_TO_TICKS(60000))) {
    }
    if (battery_charging_stdby_pin != GPIO_NUM_NC)
      chargingState_STDBY = gpio_get_level(battery_charging_stdby_pin);
    if (battery_charging_status_pin != GPIO_NUM_NC)
      chargingState_status = gpio_get_level(battery_charging_status_pin);
    ESP_LOGW(TAG, " Charging State   %d-%d\t ", chargingState_status,
             chargingState_STDBY);

    if (chargingState_STDBY == 0) {
      // Battery fullcharged
      status = battery_fullCharge;
      if (chargingState_status == 0 && chargingState_status_last == 1) {
        // experimental feature
      }
    } else if (chargingState_status != -1) {
      if (chargingState_status) {
        // Battery Not charging
        status = battery_noCharging;
      } else {
        // Battery charging
        status = battery_Charging;
      }
    }
    if (battery_charging_status != status) {
      battery_charging_status = status;
#ifdef CONFIG_UNITTEST_ENABLE_ALL
      xEventGroupSetBits(s_unittest_event_group, BATT_CHG_UNITTEST_BIT);
#endif
      battery_charging_status_change(battery_charging_status);
    }

    chargingState_STDBY_last = chargingState_STDBY;
    chargingState_status_last = chargingState_status;
    vTaskDelay(pdMS_TO_TICKS(500));
  }
}

void battery_charging_int(uint32_t ChargingPin, int32_t StandByPin) {
  gpio_config_t io_conf;
  esp_err_t err = -1;
  int i = 0;
  uint32_t buttons[2];
  if (ChargingPin != GPIO_NUM_NC) {
    userButtonsCount++;
    buttons[0] = ChargingPin;
    battery_charging_status_pin = ChargingPin;
  }
  if (StandByPin != GPIO_NUM_NC) {
    userButtonsCount++;
    buttons[1] = StandByPin;
    battery_charging_stdby_pin = StandByPin;
  }
  if (userButtonsCount) {
    batteryGPIOs = malloc(userButtonsCount * sizeof(button_state_t));
    if (batteryGPIOs) {
    } else {
      ESP_LOGE(TAG, "gpio adding failed - memory error");
      return;
    }
    // create a queue to handle gpio event from isr
    gpio_evt_queue = xQueueCreate(10, sizeof(uint32_t));

    for (i = 0; i < userButtonsCount; i++) {
      batteryGPIOs[i].button = buttons[i];
      io_conf.intr_type = GPIO_INTR_ANYEDGE;
      io_conf.pin_bit_mask = BIT64(batteryGPIOs[i].button);
      io_conf.mode = GPIO_MODE_INPUT;
      io_conf.pull_down_en = 0;
      io_conf.pull_up_en = 1;
      gpio_config(&io_conf);
    }

    // start gpio task
    BaseType_t ts =
        xTaskCreatePinnedToCore(battery_charging_task, "bat_ch_t", 4048, NULL,
                                5, &x_Batt_Chg_Task_Handle, 1);
    if (pdPASS != ts) {
      post_task_create_failed_event(__FILE__, __LINE__,
                                    esp_get_free_heap_size());
    }

    // install gpio isr service
    err = gpio_install_isr_service(ESP_INTR_FLAG_DEFAULT);
    if (err != ESP_OK)
      post_iotcore_error_event(GENERAL_ESP_ERROR_CODE, &err, sizeof(int));

    for (i = 0; i < userButtonsCount; i++) {
      // hook isr handler for specific gpio pin
      err = gpio_isr_handler_add(batteryGPIOs[i].button,
                                 battery_charging_isr_handler,
                                 (void *)batteryGPIOs[i].button);
      if (err != ESP_OK)
        post_iotcore_error_event(GENERAL_ESP_ERROR_CODE, &err, sizeof(int));

      ESP_LOGI(TAG, "%d: user button %ld added", i, batteryGPIOs[i].button);

      xQueueSend(gpio_evt_queue, &batteryGPIOs[i].button, 0);
    }
  }
}

#ifdef CONFIG_UNITTEST_ENABLE_ALL
esp_err_t unittest_battery_charging(uint32_t ChargingPin, int32_t StandByPin) {
#ifdef CONFIG_BATTERY_CHARGE_STATUS
  battery_charging_int(ChargingPin, StandByPin);
  EventBits_t uxBits;
  uxBits = xEventGroupWaitBits(s_unittest_event_group, BATT_CHG_UNITTEST_BIT,
                               pdTRUE, pdFALSE, pdMS_TO_TICKS(10000));
  if (x_Batt_Chg_Task_Handle != NULL) {
    vTaskDelete(x_Batt_Chg_Task_Handle);
  }
  if (uxBits & BATT_CHG_UNITTEST_BIT) {
    switch (battery_charging_status) {
    case battery_Charging:
    case battery_fullCharge:
      ESP_LOGW("UNITTEST", "BATTERY CHARGING OK");
      return ESP_OK;
    case battery_noCharging:
      ESP_LOGE("UNITTEST", "BATTERY CHARGING FAIL");
      return ESP_FAIL;
    case battery_noBattery:
      ESP_LOGW("BATT", "    No Battery");
      ESP_LOGE("UNITTEST", "BATTERY CHARGING FAIL");
      return ESP_FAIL;
    default:
      ESP_LOGE("UNITTEST", "BATTERY CHARGING FAIL");
      return ESP_FAIL;
    }
  } else {
    ESP_LOGE("UNITTEST", "BATTERY CHARGING FAIL");
    return ESP_FAIL;
  }

#endif
  return ESP_FAIL;
}
#endif