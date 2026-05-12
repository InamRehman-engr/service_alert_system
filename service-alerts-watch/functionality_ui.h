#ifndef FUNCTIONALITY_UI_H
#define FUNCTIONALITY_UI_H

#include <lvgl.h>

// Global UI icon variables
extern lv_obj_t* global_wifi_icon;
extern lv_obj_t* global_battery_icon;
extern lv_obj_t* global_battery_percent_label;

void create_global_battery_icon();
void create_global_wifi_icon();
void update_battery_icon_status();
void update_wifi_icon_visibility();
void update_wifi_icon_status();
void refresh();
void show_clock_screen();
void clock_screen_touch_cb(lv_event_t * e);
void handle_touch_for_swipe(lv_event_t * e);
void wifi_connect_button_handler(lv_event_t * e);

#endif // FUNCTIONALITY_UI_H