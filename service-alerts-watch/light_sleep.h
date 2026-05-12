#ifndef LIGHT_SLEEP_H
#define LIGHT_SLEEP_H
// --- Light sleep state tracking ---
extern bool light_sleep_done;
extern unsigned long clock_screen_shown_since;
extern bool rtc_time_is_valid();
extern bool rtc_matches_ntp(const struct tm* ntp_time, int threshold_sec);
extern void set_rtc_from_timeinfo(const struct tm* timeinfo);
extern void simple_rtc_sync();

bool can_enter_light_sleep();
void print_wakeup_reason();
void enter_light_sleep_40s();

#endif