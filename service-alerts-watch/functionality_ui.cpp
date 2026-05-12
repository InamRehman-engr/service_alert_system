



// Ensure LilyGoLib.h is included before any use of LilyGoLib
#include <LilyGoLib.h>
#include <lvgl.h>
#include "ui.h"
#include "functionality_ui.h"
#include <WiFi.h>
#include "wifi_credentials.h"
#include <stddef.h>
#include <cstdio>
#include "mqtt_manager.h"
#include "light_sleep.h" // for light_sleep_done, clock_screen_shown_since
#include "hardware.h" // for set_charge_current_350mA

// Global UI icon variables (declare as extern, defined in .ino)
extern lv_obj_t* global_wifi_icon;
extern lv_obj_t* global_battery_icon;
extern lv_obj_t* global_battery_percent_label;

// Externs for global state/objects
extern unsigned long last_touch_time;
extern int16_t start_x;
extern int16_t start_y;
extern bool tracking_touch;
extern bool light_sleep_done;
extern unsigned long clock_screen_shown_since;
// Externs for global state/objects
extern LilyGoWatch2022 instance;
extern int okbtn_state;
extern lv_timer_t* washroom_timer;
extern int washroom_seconds_left;
extern lv_obj_t* washroom_ui_timer;
extern lv_obj_t* washroom_ui_label4;
extern bool wifi_connecting;
extern lv_obj_t* ui_Screen2;
extern lv_obj_t* ui_Screen3;
extern lv_obj_t* ui_ScreenClock;

// Create a global WiFi icon, but only show on Screen 2 and 3
void create_global_wifi_icon() {
    global_wifi_icon = lv_label_create(lv_layer_top());
    lv_obj_set_width(global_wifi_icon, LV_SIZE_CONTENT);
    lv_obj_set_height(global_wifi_icon, LV_SIZE_CONTENT);
    lv_obj_align(global_wifi_icon, LV_ALIGN_TOP_LEFT, 5, 5);
    lv_label_set_text(global_wifi_icon, LV_SYMBOL_WIFI);
    lv_obj_set_style_text_font(global_wifi_icon, &lv_font_montserrat_18, 0);
    lv_obj_set_style_text_color(global_wifi_icon, lv_color_hex(0xFF0000), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_add_flag(global_wifi_icon, LV_OBJ_FLAG_CLICKABLE);
    // Hide by default, will be shown only on Screen 2 and 3
    lv_obj_add_flag(global_wifi_icon, LV_OBJ_FLAG_HIDDEN);
}

// Helper to update WiFi icon visibility based on current screen
void update_wifi_icon_visibility() {
    if (!global_wifi_icon) return;
    lv_obj_t* act = lv_scr_act();
    if (act == ui_Screen2 || act == ui_Screen3) {
        lv_obj_clear_flag(global_wifi_icon, LV_OBJ_FLAG_HIDDEN);
    } else {
        lv_obj_add_flag(global_wifi_icon, LV_OBJ_FLAG_HIDDEN);
    }
}

void update_wifi_icon_status() {
    if (!global_wifi_icon) return;
    
    if (wifi_connecting) {
        // Yellow color for connecting
        lv_obj_set_style_text_color(global_wifi_icon, lv_color_hex(0xFFAA00), LV_PART_MAIN | LV_STATE_DEFAULT);
    } else if (WiFi.status() == WL_CONNECTED) {
        // Green color for connected
        lv_obj_set_style_text_color(global_wifi_icon, lv_color_hex(0x00AA00), LV_PART_MAIN | LV_STATE_DEFAULT);
    } else {
        // Red color for disconnected
        lv_obj_set_style_text_color(global_wifi_icon, lv_color_hex(0xFF0000), LV_PART_MAIN | LV_STATE_DEFAULT);
    }
}

void refresh(){
        if (okbtn_state == 1) {
            // Reset and stop timer, set labels
            if (washroom_timer) {
                lv_timer_del(washroom_timer);
                washroom_timer = NULL;
            }
            washroom_seconds_left = 0;
            if (washroom_ui_timer && washroom_ui_label4) {
                lv_label_set_text(washroom_ui_timer, "waiting for alert...");
                lv_label_set_text(washroom_ui_label4, "Time to Reach");
            }
            okbtn_state = 0;
        } 
}



// Create a global Battery icon and a separate percentage label, show on all screens
void create_global_battery_icon() {
    global_battery_icon = lv_label_create(lv_layer_top());
    lv_obj_set_width(global_battery_icon, LV_SIZE_CONTENT);
    lv_obj_set_height(global_battery_icon, LV_SIZE_CONTENT);
    lv_obj_align(global_battery_icon, LV_ALIGN_TOP_RIGHT, -5, 5);
    lv_label_set_text(global_battery_icon, LV_SYMBOL_BATTERY_FULL);
    lv_obj_set_style_text_font(global_battery_icon, &lv_font_montserrat_18, 0);
    lv_obj_set_style_text_color(global_battery_icon, lv_color_hex(0x00AA00), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_add_flag(global_battery_icon, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_clear_flag(global_battery_icon, LV_OBJ_FLAG_HIDDEN);

    // Create percentage label to the left of the icon
    global_battery_percent_label = lv_label_create(lv_layer_top());
    lv_obj_set_width(global_battery_percent_label, LV_SIZE_CONTENT);
    lv_obj_set_height(global_battery_percent_label, LV_SIZE_CONTENT);
    lv_obj_align_to(global_battery_percent_label, global_battery_icon, LV_ALIGN_OUT_LEFT_MID, -5, 0);
    lv_label_set_text(global_battery_percent_label, "100%");
    lv_obj_set_style_text_font(global_battery_percent_label, &lv_font_montserrat_16, 0);
    lv_obj_set_style_text_color(global_battery_percent_label, lv_color_hex(0x00AA00), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_add_flag(global_battery_percent_label, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_clear_flag(global_battery_percent_label, LV_OBJ_FLAG_HIDDEN);
}

// Helper to update Battery icon color and percentage text
void update_battery_icon_status() {
    int percent = 0;
    if (!global_battery_icon || !global_battery_percent_label) return;
    percent = get_battery_percent();
    if (percent == -1) {
        Serial.println("[WARNING APX2101] Battery percentage unavailable.");
    }
    // Show charging icon if charging
    if (instance.pmu.isCharging()) {
        lv_label_set_text(global_battery_icon, LV_SYMBOL_CHARGE);
        lv_obj_set_style_text_color(global_battery_icon, lv_color_hex(0x00AAFF), LV_PART_MAIN | LV_STATE_DEFAULT); // Blue for charging
        lv_obj_set_style_text_color(global_battery_percent_label, lv_color_hex(0x00AAFF), LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_label_set_text(global_battery_percent_label, "CHG");
        return;
    }
    // Only icon, no text status
    lv_color_t color;
    if (percent <= 15) {
        color = lv_color_hex(0xFF0000); // Red
    } else if (percent <= 40) {
        color = lv_color_hex(0xFFAA00); // Orange
    } else {
        color = lv_color_hex(0x00AA00); // Green
    }
    lv_obj_set_style_text_color(global_battery_icon, color, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(global_battery_percent_label, color, LV_PART_MAIN | LV_STATE_DEFAULT);
    char percent_str[8];
    snprintf(percent_str, sizeof(percent_str), "%d%%", percent);
    lv_label_set_text(global_battery_percent_label, percent_str);
}

// Custom touch event handler for swipe detection
void handle_touch_for_swipe(lv_event_t * e) {
    lv_indev_t * indev = lv_indev_get_act();
    if(indev == NULL) return;
    
    lv_point_t point;
    lv_indev_get_point(indev, &point);
    
    lv_event_code_t code = lv_event_get_code(e);
    
    if(code == LV_EVENT_PRESSED) {
        // Block swipe if waiting for alert on Screen2
        if (block_swipe_and_lockscreen && lv_scr_act() == ui_Screen2) return;
        // Only update last_touch_time on real user touch
        last_touch_time = millis();
        // Record starting position when touch begins
        start_x = point.x;
        start_y = point.y;
        tracking_touch = true;
        Serial.printf("Touch started at x=%d, y=%d\n", point.x, point.y);
    }
    else if(code == LV_EVENT_PRESSING && tracking_touch) {
        // Optionally track movement during the press for visual feedback
        // This could be used to show a hint that swiping is happening
    }
    if(code == LV_EVENT_RELEASED && tracking_touch) {
        if (block_swipe_and_lockscreen && lv_scr_act() == ui_Screen2) return;
        // Calculate the distance moved
        int16_t dx = point.x - start_x;
        int16_t dy = point.y - start_y;
        tracking_touch = false;
        // Determine if it's a horizontal swipe (more horizontal than vertical movement)
        if(abs(dx) > abs(dy) && abs(dx) > 40) { // At least 40 pixels swipe to count (optimized for better responsiveness)
            if(dx > 0) {
                // Right swipe
                Serial.println("RIGHT SWIPE DETECTED!");
                if(lv_scr_act() == ui_Screen2) {
                    lv_scr_load_anim(ui_Screen1, LV_SCR_LOAD_ANIM_MOVE_RIGHT, 300, 0, false);
                }
                else if(lv_scr_act() == ui_Screen3) {
                    // Hide keyboard if visible before changing screens
                    if (!lv_obj_has_flag(ui_Keyboard2, LV_OBJ_FLAG_HIDDEN)) {
                        lv_obj_add_flag(ui_Keyboard2, LV_OBJ_FLAG_HIDDEN);
                        // Restore original positions if needed
                        if (positions_stored) {
                            lv_obj_set_y(ui_ConnectButton, original_connect_btn_y);
                            lv_obj_set_y(ui_StatusLabel, original_status_label_y);
                        }
                    }
                    lv_scr_load_anim(ui_Screen2, LV_SCR_LOAD_ANIM_MOVE_RIGHT, 300, 0, false);
                    // Reset WiFi status display and hide it when leaving the screen
                    lv_label_set_text(ui_StatusLabel, "");
                    if (!wifi_connecting) {
                        lv_obj_add_flag(ui_StatusLabel, LV_OBJ_FLAG_HIDDEN);
                        lv_obj_add_flag(ui_WifiIcon, LV_OBJ_FLAG_HIDDEN);
                    }
                }
            } else {
                // Left swipe
                Serial.println("LEFT SWIPE DETECTED!");
                if(lv_scr_act() == ui_Screen1) {
                    lv_scr_load_anim(ui_Screen2, LV_SCR_LOAD_ANIM_MOVE_LEFT, 300, 0, false);
                }
                else if(lv_scr_act() == ui_Screen2) {
                    lv_scr_load_anim(ui_Screen3, LV_SCR_LOAD_ANIM_MOVE_LEFT, 300, 0, false);
                }
            }
        }
        Serial.printf("Touch ended at x=%d, y=%d (moved dx=%d, dy=%d)\n", 
                     point.x, point.y, dx, dy);
    }
}


// Show the clock screen and set up touch to return
void show_clock_screen() {
    if (ui_ScreenClock) {
        Serial.println("[DEBUG] ui_ScreenClock is valid. Loading clock screen.");
        lv_scr_load(ui_ScreenClock);
        // Lower brightness for power saving when clock is shown
        instance.setBrightness(30); // Set to a low value, adjust as needed
        // Remove any previous callbacks to avoid duplicates
        lv_obj_remove_event_cb(ui_ScreenClock, NULL);
        lv_obj_add_event_cb(ui_ScreenClock, clock_screen_touch_cb, LV_EVENT_PRESSED, NULL);
    } else {
        Serial.println("[ERROR] ui_ScreenClock is NULL! Cannot load clock screen.");
    }
}

// Return to Screen1 on any touch from clock screen
void clock_screen_touch_cb(lv_event_t * e) {
    if (lv_scr_act() == ui_ScreenClock) {
        last_touch_time = millis();
        // Restore normal brightness when leaving clock screen
        instance.setBrightness(30);
        lv_scr_load(ui_Screen1);
        // Reset light sleep state when leaving clock screen
        light_sleep_done = false;
        clock_screen_shown_since = 0;
    }
}

// WiFi connection handler
void wifi_connect_button_handler(lv_event_t * e) {
    lv_event_code_t code = lv_event_get_code(e);
    
    if(code == LV_EVENT_CLICKED) {
        // Get the text from the text fields
        const char* ssid_text = lv_textarea_get_text(ui_TextArea1);
        const char* password_text = lv_textarea_get_text(ui_TextArea2);
        
        Serial.printf("WiFi Connect button clicked. SSID: %s\n", ssid_text);
        Serial.printf("Password length: %d\n", strlen(password_text));
        
        // Make sure status label is visible - force unhide and restore position
        lv_obj_clear_flag(ui_StatusLabel, LV_OBJ_FLAG_HIDDEN);
        lv_obj_set_y(ui_StatusLabel, original_status_label_y);
        lv_obj_move_foreground(ui_StatusLabel);
        
        // Set initial status text so user knows something is happening
        lv_label_set_text(ui_StatusLabel, "Initializing...");
        lv_obj_set_style_text_color(ui_StatusLabel, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
        Serial.println("Connect button pressed - status label should be visible");
        
        // Check for empty fields (this should not happen since button is hidden if fields are empty)
        if (strlen(ssid_text) == 0 || strlen(password_text) == 0) {
            lv_label_set_text(ui_StatusLabel, "Error: SSID and Password required");
            lv_obj_set_style_text_color(ui_StatusLabel, lv_color_hex(0xFF0000), LV_PART_MAIN | LV_STATE_DEFAULT);
            return;
        }
        
        // Copy to our buffer
        strncpy(wifi_ssid, ssid_text, sizeof(wifi_ssid) - 1);
        strncpy(wifi_password, password_text, sizeof(wifi_password) - 1);
        
        // Ensure null termination
        wifi_ssid[sizeof(wifi_ssid) - 1] = '\0';
        wifi_password[sizeof(wifi_password) - 1] = '\0';
        
        // Start WiFi connection
        connect_to_wifi(wifi_ssid, wifi_password);

    }
}
