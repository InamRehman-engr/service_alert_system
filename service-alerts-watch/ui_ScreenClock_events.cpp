#include "ui_ScreenClock.h"
#include <time.h>
#include <WiFi.h>

// Call this periodically to update the digital clock and battery status
#include "LilyGoLib.h" // for instance
// --- RTC/NTP/TIMEZONE LOGIC MOVED FROM .INO ---
#include <HTTPClient.h>
#include <Preferences.h>
#include <String.h>

// Helper: Check if RTC time is valid (year >= 2020)
bool rtc_time_is_valid() {
    time_t now = time(NULL);
    struct tm timeinfo;
    localtime_r(&now, &timeinfo);
    return (timeinfo.tm_year + 1900 >= 2020);
}

// Helper: Compare RTC and NTP times, return true if within threshold (seconds)
bool rtc_matches_ntp(const struct tm* ntp_time, int threshold_sec) {
    time_t rtc_now = time(NULL);
    time_t ntp_epoch = mktime((struct tm*)ntp_time);
    return abs((long)(rtc_now - ntp_epoch)) <= threshold_sec;
}

// Set ESP32 RTC from struct tm (after NTP sync)
void set_rtc_from_timeinfo(const struct tm* timeinfo) {
    struct timeval tv;
    tv.tv_sec = mktime((struct tm*)timeinfo);
    tv.tv_usec = 0;
    settimeofday(&tv, NULL);
    Serial.println("[RTC] ESP32S3 RTC set from NTP time");
}


// Simple RTC sync: Set fixed timezone, sync NTP, and update RTC if successful
void simple_rtc_sync() {
    // Try PKT-5 first
    String tz_env = "Asia/Karachi"; // PKT-5
    Serial.print("[simple_rtc_sync] Setting TZ to: ");
    Serial.println(tz_env);
    setenv("TZ", tz_env.c_str(), 1);
    tzset();
    Serial.println("[simple_rtc_sync] TZ and tzset applied. Now calling configTime...");
    configTime(0, 0, "pool.ntp.org");
    delay(2000); // Wait for NTP sync
    struct tm ntp_timeinfo;
    if (getLocalTime(&ntp_timeinfo)) {
        Serial.printf("[simple_rtc_sync] getLocalTime (PKT-5) success. Raw: %04d-%02d-%02d %02d:%02d:%02d\n", ntp_timeinfo.tm_year+1900, ntp_timeinfo.tm_mon+1, ntp_timeinfo.tm_mday, ntp_timeinfo.tm_hour, ntp_timeinfo.tm_min, ntp_timeinfo.tm_sec);
        set_rtc_from_timeinfo(&ntp_timeinfo);
        Serial.print("[RTC] RTC set from NTP. Timezone: PKT-5\n");
    } else {
        Serial.print("[NTP] Failed to get time\n");
    }
}

void ui_ScreenClock_update_from_rtc() {
    time_t now = time(NULL);
    now = 5 * 3600 + now; // Asia/Karachi offset
    struct tm timeinfo;
    localtime_r(&now, &timeinfo);
    if (timeinfo.tm_year + 1900 >= 2020) {
        static char buf[16];
        int hour12 = timeinfo.tm_hour % 12;
        if (hour12 == 0) hour12 = 12;
        const char* ampm = (timeinfo.tm_hour < 12) ? "AM" : "PM";
        snprintf(buf, sizeof(buf), "%02d:%02d:%02d %s", hour12, timeinfo.tm_min, timeinfo.tm_sec, ampm);
        ui_ScreenClock_update_time(buf);
    } else {
        ui_ScreenClock_update_time("--:--:--");
    }
    // No battery label or text, only icon is used globally
}
