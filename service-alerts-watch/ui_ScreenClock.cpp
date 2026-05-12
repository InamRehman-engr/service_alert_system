// Renamed from .c to .cpp for C++ compatibility

#include <Arduino.h>
#include "ui.h"
#include <lvgl.h>

lv_obj_t * ui_ScreenClock = NULL;
lv_obj_t * ui_LabelClock = NULL;

void ui_ScreenClock_screen_init(void) {
	ui_ScreenClock = lv_obj_create(NULL);
	lv_obj_set_style_bg_color(ui_ScreenClock, lv_color_hex(0x000000), LV_PART_MAIN);
	lv_obj_set_style_bg_opa(ui_ScreenClock, LV_OPA_COVER, LV_PART_MAIN);

	// Digital clock label
	ui_LabelClock = lv_label_create(ui_ScreenClock);
	lv_obj_set_style_text_color(ui_LabelClock, lv_color_hex(0x00FF00), LV_PART_MAIN);
	lv_obj_set_style_text_font(ui_LabelClock, &lv_font_montserrat_36, LV_PART_MAIN);
	lv_obj_align(ui_LabelClock, LV_ALIGN_CENTER, 0, 0);
	lv_label_set_text(ui_LabelClock, "00:00:00");

}

void ui_ScreenClock_update_time(const char* timeStr) {
	if (ui_LabelClock) {
		lv_label_set_text(ui_LabelClock, timeStr);
	} else {
		Serial.println("[ERROR] ui_LabelClock is NULL in ui_ScreenClock_update_time!");
	}
}
