#include <LilyGoLib.h> // for instance
#include "esp_pm.h"
#include <LV_Helper.h>
#include <HTTPClient.h>
#include <Preferences.h>
#include"mqtt_manager.h"
#include <time.h>
#include "ui.h"
#include "wifi_credentials.h"
#include <esp_wifi.h>
#include <ArduinoJson.h> // For JSON parsing
#include "light_sleep.h"
#include "functionality_ui.h"
#include "hardware.h"

extern const int SAMPLE_RATE;


extern bool rtc_time_is_valid();
extern bool rtc_matches_ntp(const struct tm* ntp_time, int threshold_sec);
extern void set_rtc_from_timeinfo(const struct tm* timeinfo);
extern void simple_rtc_sync();


// Forward declaration for clock update
void ui_ScreenClock_update_from_rtc();


// --- Inactivity/Clock screen logic ---
unsigned long last_touch_time = 0; // For inactivity detection
static bool clock_shown_due_to_inactivity = false;
void show_clock_screen();
void clock_screen_touch_cb(lv_event_t * e);

// Track touch for swipe detection
int16_t start_x = 0;
int16_t start_y = 0;
bool tracking_touch = false;
bool rtc_synced = false;


// WiFi related variables
char wifi_ssid[64] = "";
char wifi_password[64] = "";
bool wifi_connecting = false;
unsigned long wifi_start_time = 0;
const unsigned long wifi_timeout = 20000; // 20 seconds timeout
int g_user_wifi_retry_count = 0;
char g_last_user_ssid[64] = "";
char g_last_user_password[64] = "";
bool g_user_wifi_pending_disconnect = false;


// Global WiFi status icon that appears on all screens
lv_obj_t* global_wifi_icon = NULL;
lv_obj_t* global_battery_icon = NULL;
lv_obj_t* global_battery_percent_label = NULL;


// --- MQTT Setup ---
WiFiClient espClient;
PubSubClient mqttClient(espClient);
int okbtn_state = 0; // 0 = not pressed, 1 = pressed
int last_alert_button_arr[6] = {0}; // Store last received array
bool block_swipe_and_lockscreen = false;
bool alert_ads_received = false;
const char* mqtt_server = "iotcore.cowlar.com"; // Change to your broker if needed
const int mqtt_port = 1883;
const char* mqtt_topic = "alert/button";
const char* mqtt_user = "dockersim-dispenser"; // Set your MQTT username
const char* mqtt_pass = "CowlarGeyser7890"; // Set your MQTT password


// --- Light sleep state tracking ---
bool light_sleep_done = false;
unsigned long clock_screen_shown_since = 0;


// --- Washroom timer state for alert/button ---
lv_timer_t * washroom_timer = NULL;
lv_obj_t * washroom_ui_timer = NULL;
lv_obj_t * washroom_ui_label4 = NULL;
int washroom_seconds_left = 0;
unsigned long last_clock_update = 0;
bool was_clock_screen = false;


void setup(){
    // Enable Dynamic Frequency Scaling (DFS) and automatic light sleep
    setCpuFrequencyMhz(80); // Set lower base frequency (can be 80 or 160)
    esp_pm_config_esp32_t pm_config;
    pm_config.max_freq_mhz = 240; // Max CPU frequency
    pm_config.min_freq_mhz = 80;  // Min CPU frequency
    pm_config.light_sleep_enable = true;
    esp_pm_configure(&pm_config);
    // Reset light sleep state on boot/wake
    light_sleep_done = false;
    clock_screen_shown_since = 0;
    print_wakeup_reason();
    // Initialize inactivity timer
    last_touch_time = millis();
    Serial.begin(115200);
    Serial.println("T-Watch S3 initializing...");
    
    instance.begin();                  // Init hardware
    beginLvglHelper(instance);         // Init LVGL

    // Initialize DRV2605 haptic with correct I2C pins (SDA=10, SCL=11)
    instance.drv.begin(Wire, 10, 11);
    // Enable speaker amplifier power (use public method)

    // Turn on the audio power (speaker enable pin)
    instance.powerControl(POWER_SPEAK, true);

    // Setting up the charge current to 350mA default: 300mA
    set_charge_current_350mA();
    
     // ---- FIXED I2S INIT (instead of setVolume/open) ----
    if (!instance.player.begin(I2S_MODE_STD, SAMPLE_RATE,
                               I2S_DATA_BIT_WIDTH_16BIT,
                               I2S_SLOT_MODE_MONO)) {
        Serial.println("Failed to init I2S!");
        return;
    }

    // Set up LVGL logging
    Serial.println("Setting up display...");

    // Configure input device for gestures
    Serial.println("Starting UI initialization");
    ui_init();   // Initialize and load the startup screen from SquareLine

    // Add our custom swipe handler directly to the screens after UI init, with null checks
    if (ui_Screen1) {
        lv_obj_add_event_cb(ui_Screen1, handle_touch_for_swipe, LV_EVENT_PRESSED, NULL);
        lv_obj_add_event_cb(ui_Screen1, handle_touch_for_swipe, LV_EVENT_RELEASED, NULL);
        lv_obj_add_event_cb(ui_Screen1, handle_touch_for_swipe, LV_EVENT_PRESSING, NULL);
    } else {
        Serial.println("[ERROR] ui_Screen1 is NULL after ui_init()");
    }
    if (ui_Screen2) {
        lv_obj_add_event_cb(ui_Screen2, handle_touch_for_swipe, LV_EVENT_PRESSED, NULL);
        lv_obj_add_event_cb(ui_Screen2, handle_touch_for_swipe, LV_EVENT_RELEASED, NULL);
        lv_obj_add_event_cb(ui_Screen2, handle_touch_for_swipe, LV_EVENT_PRESSING, NULL);
    } else {
        Serial.println("[ERROR] ui_Screen2 is NULL after ui_init()");
    }
    if (ui_Screen3) {
        lv_obj_add_event_cb(ui_Screen3, handle_touch_for_swipe, LV_EVENT_PRESSED, NULL);
        lv_obj_add_event_cb(ui_Screen3, handle_touch_for_swipe, LV_EVENT_RELEASED, NULL);
        lv_obj_add_event_cb(ui_Screen3, handle_touch_for_swipe, LV_EVENT_PRESSING, NULL);
    } else {
        Serial.println("[ERROR] ui_Screen3 is NULL after ui_init()");
    }
    if (ui_ConnectButton) {
        lv_obj_remove_event_cb(ui_ConnectButton, NULL);
        lv_obj_add_event_cb(ui_ConnectButton, wifi_connect_button_handler, LV_EVENT_CLICKED, NULL);
        Serial.println("WiFi Connect button handler registered");
    } else {
        Serial.println("[ERROR] ui_ConnectButton is NULL after ui_init()");
    }
    create_global_wifi_icon();
    create_global_battery_icon();

    Serial.println("UI initialization complete");
    Serial.println("Custom swipe detection enabled - Swipe left/right to change screens");

    // Start non-blocking WiFi credential cycling on startup
    start_try_connect_all_stored_wifi();
    Serial.printf("[WIFI] Loaded %d credentials from NVS.\n", wifi_cred_state.count);

    // instance.setBrightness(DEVICE_MAX_BRIGHTNESS_LEVEL);
    instance.setBrightness(30);

    // MQTT setup
    mqttClient.setServer(mqtt_server, mqtt_port);
    mqttClient.setCallback(mqtt_callback);
    // Subscribe to alert/ads with a separate callback
    mqttClient.subscribe("alert/ads");
}


void loop() {
    // Light sleep logic: only if all conditions are met and clock screen is active
    static unsigned long last_sleep_check = 0;
    if (can_enter_light_sleep() && millis() - last_sleep_check > 1000) {
        last_sleep_check = millis();
        enter_light_sleep_40s();
    }
    refresh();
    // Always try to reconnect using stored credentials if WiFi is disconnected and not already trying
    if (WiFi.status() != WL_CONNECTED && !wifi_cred_state.in_progress && !wifi_connecting) {
        start_try_connect_all_stored_wifi();
        Serial.println("[WIFI] WiFi disconnected, retrying with stored credentials...");
    }
    // Process stored WiFi connection attempts
    if (wifi_cred_state.in_progress) {
        if (process_try_connect_all_stored_wifi()) {
            Serial.printf("[WIFI] Connected to stored WiFi: %s\n", WiFi.SSID().c_str());
            if (!rtc_synced) {
                simple_rtc_sync(); // Sync timezone and RTC on first connect
                rtc_synced = true;
            }
        }
    }

    // --- Robust MQTT reconnect logic ---
    bool wifi_now_connected = (WiFi.status() == WL_CONNECTED);
    static unsigned long last_mqtt_attempt = 0;
    if (wifi_now_connected) {
        if (!mqttClient.connected()) {
            if (millis() - last_mqtt_attempt > 2000) { // Try every 2 seconds
                mqtt_reconnect();
                last_mqtt_attempt = millis();
            }
        } else {
            mqttClient.loop();
        }
    }

    // UI clock update logic only
    if (lv_scr_act() == ui_ScreenClock) {
        if (millis() - last_clock_update > 1000) {
            last_clock_update = millis();
            ui_ScreenClock_update_from_rtc();
        }
        was_clock_screen = true;
    } else {
        was_clock_screen = false;
    }
    // Handle user-initiated WiFi connection (from UI)
    if (wifi_connecting) {
        check_wifi_status();
    }
    bool keyboard_hidden = true;
    extern lv_obj_t * ui_Keyboard2;
    extern lv_obj_t * ui_Keyboard;
    if ((ui_Keyboard2 && !lv_obj_has_flag(ui_Keyboard2, LV_OBJ_FLAG_HIDDEN)) ||
        (ui_Keyboard && !lv_obj_has_flag(ui_Keyboard, LV_OBJ_FLAG_HIDDEN))) {
        keyboard_hidden = false;
        last_touch_time = millis();
    }
    if (!block_swipe_and_lockscreen && millis() - last_touch_time > 10000 && keyboard_hidden) { // 10 seconds
        if (!clock_shown_due_to_inactivity && lv_scr_act() != ui_ScreenClock) {
            Serial.println("[DEBUG] Inactivity timeout reached. Attempting to show clock screen.");
            show_clock_screen();
            clock_shown_due_to_inactivity = true;
        }
    } else {
        clock_shown_due_to_inactivity = false;
    }

    lv_timer_handler();
    update_wifi_icon_status();
    update_wifi_icon_visibility();
    update_battery_icon_status();
    check_and_beep_low_battery();
    delay(2);
}
