#ifndef UI_SCREENCLOCK_H
#define UI_SCREENCLOCK_H

#include <lvgl.h>

extern lv_obj_t * ui_ScreenClock;

extern lv_obj_t * ui_LabelClock;

#ifdef __cplusplus
extern "C" {
#endif

void ui_ScreenClock_screen_init(void);
void ui_ScreenClock_update_time(const char* timeStr);
void ui_ScreenClock_update_from_rtc(void);

#ifdef __cplusplus
}
#endif

#endif
