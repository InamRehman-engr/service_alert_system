
#include <esp_sleep.h>
#include "light_sleep.h"
#include <WiFi.h>
#include <PubSubClient.h>
#include <lvgl.h>
#include <Arduino.h>
#include "mqtt_manager.h"
#include "ui_ScreenClock.h"
#include "esp_bt.h"
#include "driver/adc.h"
#include "hardware.h"
#include "esp_adc_cal.h"
// Use correct type for instance
#include <LilyGoLib.h>
extern LilyGoWatch2022 instance;

// Add extern for rtc_synced
extern bool rtc_synced;

bool can_enter_light_sleep() {
    // Do not enter sleep if charging
    if (instance.pmu.isCharging()) return false;
    // WiFi connected
    if (WiFi.status() != WL_CONNECTED) return false;
    // MQTT connected
    if (!mqttClient.connected()) return false;
    // RTC synced (use rtc_synced global, not extern)
    if (!rtc_synced) return false;
    // Clock screen is active
    extern lv_obj_t * ui_ScreenClock;
    if (lv_scr_act() != ui_ScreenClock) {
        clock_screen_shown_since = 0; // Reset timer if not on clock screen
        return false;
    }
    // Track how long clock screen has been shown
    if (clock_screen_shown_since == 0) {
        clock_screen_shown_since = millis();
        return false;
    }
    // Only allow sleep if clock screen has been shown for at least 5 seconds
    if (millis() - clock_screen_shown_since < 5000) return false;
    // Only allow sleep once per clock screen appearance
    if (light_sleep_done) return false;
    return true;
}

void print_wakeup_reason() {
    esp_sleep_wakeup_cause_t wakeup_reason = esp_sleep_get_wakeup_cause();
    switch (wakeup_reason) {
        case ESP_SLEEP_WAKEUP_EXT0: Serial.println("[SLEEP] Wakeup cause: EXT0"); break;
        case ESP_SLEEP_WAKEUP_EXT1: Serial.println("[SLEEP] Wakeup cause: EXT1"); break;
        case ESP_SLEEP_WAKEUP_TIMER: Serial.println("[SLEEP] Wakeup cause: TIMER"); break;
        case ESP_SLEEP_WAKEUP_TOUCHPAD: Serial.println("[SLEEP] Wakeup cause: TOUCHPAD"); break;
        case ESP_SLEEP_WAKEUP_ULP: Serial.println("[SLEEP] Wakeup cause: ULP"); break;
        case ESP_SLEEP_WAKEUP_GPIO: Serial.println("[SLEEP] Wakeup cause: GPIO"); break;
        case ESP_SLEEP_WAKEUP_UART: Serial.println("[SLEEP] Wakeup cause: UART"); break;
        case ESP_SLEEP_WAKEUP_WIFI: Serial.println("[SLEEP] Wakeup cause: WIFI"); break;
        case ESP_SLEEP_WAKEUP_COCPU: Serial.println("[SLEEP] Wakeup cause: COCPU"); break;
        case ESP_SLEEP_WAKEUP_COCPU_TRAP_TRIG: Serial.println("[SLEEP] Wakeup cause: COCPU_TRAP_TRIG"); break;
        case ESP_SLEEP_WAKEUP_BT: Serial.println("[SLEEP] Wakeup cause: BT"); break;
        case ESP_SLEEP_WAKEUP_UNDEFINED:
        default:
            Serial.println("[SLEEP] Wakeup cause: UNDEFINED");
            break;
    }
}
// Enter ESP32 light sleep for 40 seconds
void enter_light_sleep_40s() {
    Serial.println("[SLEEP] Entering light sleep for 40 seconds...");
    delay(100); // Short delay before sleep
    light_sleep_done = true;
    // Prepare wakeup timer
    esp_sleep_enable_timer_wakeup(40 * 1000000ULL); // 40 seconds in microseconds
    
    // Go to sleep
    esp_light_sleep_start();    
}