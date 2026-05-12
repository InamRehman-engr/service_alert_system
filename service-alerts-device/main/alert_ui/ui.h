#ifndef UI_H
#define UI_H

#include "lvgl.h"

// Screen types
typedef enum {
    SCREEN_ALERT = 0,
    SCREEN_ADS = 1
} screen_type_t;

void alert_ui_init(lv_display_t *disp);
void set_wifi_icon_status(int status);
void switch_to_ads_screen(const char* ads_data);
void switch_to_alert_screen(void);

#endif // UI_H