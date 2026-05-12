#ifndef _button_h_
#define _button_h_
#include "esp_event.h"
#include "sdkconfig.h"
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define BUTTON_INCLUDED

typedef enum { PULL_UP, PULL_DOWN, PULL_NONE } GPIO_CONFIG;

void button_int(uint32_t *buttons, int32_t numberOfButtons,
                GPIO_CONFIG config[]);
void buttonPressedCallbackShort(void (*cb_function)(int32_t, int32_t));
void buttonPressedCallbackMedium(void (*cb_function)(int32_t, int32_t));
void buttonPressedCallbackLong(void (*cb_function)(int32_t, int32_t));
void buttonPressedCallbackState_high(void (*cb_function)(int32_t, int32_t));
void buttonPressedCallbackState_low(void (*cb_function)(int32_t, int32_t));
void buttonPressed_ext(uint32_t btn, int32_t prssedTime);
int getButtonState(uint32_t buttonToGet);

#ifdef CONFIG_UNITTEST_ENABLE_ALL
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include <esp_err.h>

extern EventGroupHandle_t s_unittest_event_group;
extern const int BUTTON_UNITTEST_BIT;
esp_err_t unittest_button(char *unit);
#endif

#endif //_button_h_