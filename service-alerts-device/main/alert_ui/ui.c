#include "lvgl.h"
#include "ui.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "esp_timer.h"
#include "app_rtc.h"

static lv_obj_t *time_label;
static lv_obj_t *alert_text_obj;  // Global reference for animation
static lv_obj_t *wifi_icon;

// Screen management
static lv_display_t *display_ref = NULL;
static lv_obj_t *alert_screen = NULL;
static lv_obj_t *ads_screen = NULL;
static screen_type_t current_screen = SCREEN_ALERT;

// Global variables for animation state
static int32_t current_hue = 15;
static lv_obj_t *ads_text_obj = NULL;  // Global reference for ads text animation
static lv_timer_t *ads_switch_timer = NULL;  // Timer for switching between screen styles
static volatile bool ads_screen_style = false;  // false = style1, true = style2
static char *current_ads_data = NULL;  // Store current ads data for switching

// Elapsed time tracking for ads screen
static lv_obj_t *ads_timer_label = NULL;  // Global reference for elapsed time label
static lv_timer_t *ads_elapsed_timer = NULL;  // Timer for updating elapsed time
static volatile uint32_t ads_elapsed_seconds = 0;  // Elapsed time counter


// Global WiFi status (thread-safe updates)
static volatile int current_wifi_status = 0;
static int last_displayed_wifi_status = -1;

// WiFi icon using Unicode symbol
static const char* wifi_symbol = LV_SYMBOL_WIFI;

// Forward declarations
static void update_wifi_icon_display(void);
static void create_common_elements(lv_obj_t *screen);
static void create_ads_screen(const char* ads_data);
static void create_ads_screen1(const char* ads_data);
static void create_ads_screen2(const char* ads_data);



// Timer callback for updating elapsed time on ads screen (thread-safe)
static void ads_elapsed_timer_cb(lv_timer_t * timer)
{
    if (!ads_timer_label || current_screen != SCREEN_ADS) {
        return;
    }
    
    // Ensure object is still valid
    if (!lv_obj_is_valid(ads_timer_label)) {
        return;
    }
    
    // Increment elapsed time
    ads_elapsed_seconds++;
    
    // Format elapsed time as MM:SS
    uint32_t minutes = ads_elapsed_seconds / 60;
    uint32_t seconds = ads_elapsed_seconds % 60;
    
    static char elapsed_buffer[16];
    snprintf(elapsed_buffer, sizeof(elapsed_buffer), "%02lu:%02lu", 
             (unsigned long)minutes, (unsigned long)seconds);
    
    // Update the timer label
    lv_label_set_text(ads_timer_label, elapsed_buffer);
    
    printf("Ads elapsed time: %s\n", elapsed_buffer);
}

// Timer callback for switching ads screen styles (thread-safe)
static void ads_switch_timer_cb(lv_timer_t * timer)
{
    if (!display_ref || current_screen != SCREEN_ADS) {
        return;
    }
    
    // Increment elapsed time first
    ads_elapsed_seconds++;
    
    // Toggle screen style
    ads_screen_style = !ads_screen_style;
    
    if (!ads_screen_style) {
        // Use style 1: white background, red text
        create_ads_screen1(current_ads_data);
        printf("Switched to ads screen style 1 (white bg, red text)\n");
    } else {
        // Use style 2: red background, white text  
        create_ads_screen2(current_ads_data);
        printf("Switched to ads screen style 2 (red bg, white text)\n");
    }
    
    // Update elapsed time display on the newly created screen
    if (ads_timer_label && lv_obj_is_valid(ads_timer_label)) {
        uint32_t minutes = ads_elapsed_seconds / 60;
        uint32_t seconds = ads_elapsed_seconds % 60;
        
        static char elapsed_buffer[16];
        snprintf(elapsed_buffer, sizeof(elapsed_buffer), "%02lu:%02lu", 
                 (unsigned long)minutes, (unsigned long)seconds);
        
        lv_label_set_text(ads_timer_label, elapsed_buffer);
        printf("Updated timer display: %s\n", elapsed_buffer);
    }
    
    // Load the newly created screen
    lv_scr_load(ads_screen);
}

// Main function to create ads screen with timer switching
static void create_ads_screen(const char* ads_data)
{
    // Store ads data for timer callback
    if (current_ads_data) {
        free(current_ads_data);
    }
    current_ads_data = ads_data ? strdup(ads_data) : NULL;
    
    // Stop existing timer if running
    if (ads_switch_timer) {
        lv_timer_del(ads_switch_timer);
        ads_switch_timer = NULL;
    }
    if (ads_elapsed_timer) {
        lv_timer_del(ads_elapsed_timer);
        ads_elapsed_timer = NULL;
    }
    
    // Reset elapsed time counter
    ads_elapsed_seconds = 0;
    
    // Create initial screen (style 1)
    ads_screen_style = false;
    create_ads_screen1(ads_data);
    
    // Start timer to switch between styles every 1 second (also updates elapsed time)
    ads_switch_timer = lv_timer_create(ads_switch_timer_cb, 1000, NULL);
    
    printf("Started ads screen switching timer (1000ms interval)\n");
}

// Animation callback for hue (orange range)
static void hue_anim_cb(void * var, int32_t v)
{
    current_hue = v;  // Update the global variable with animated value
    lv_obj_t * obj = (lv_obj_t*)var;
    
    // Create HSV color with animated hue value
    lv_color_hsv_t hsv = {v, 100, 100};  // Use 'v' parameter, not current_hue
    lv_color_t color = lv_color_hsv_to_rgb(hsv.h, hsv.s, hsv.v);
    
    // Update text color
    lv_obj_set_style_text_color(obj, color, 0);
}


// Function to update time using RTC and WiFi status
static void update_time_label(lv_timer_t * timer)
{
    time_t now;
    struct tm timeinfo;
    static char time_buffer[32];  // Increased buffer size to avoid truncation warning
    
    // Get current time from RTC (already includes timezone adjustment)
    time_local(&now);
    
    // Use gmtime_r in a thread-safe manner with local copy
    if (gmtime_r(&now, &timeinfo) == NULL) {
        return;  // Exit if time conversion fails
    }
    
    // Convert to 12-hour format
    int display_hours = timeinfo.tm_hour;
    const char* ampm = "AM";
    
    if (timeinfo.tm_hour == 0) {
        display_hours = 12;
    } else if (timeinfo.tm_hour > 12) {
        display_hours = timeinfo.tm_hour - 12;
        ampm = "PM";
    } else if (timeinfo.tm_hour == 12) {
        ampm = "PM";
    }
    
    // Format time string in buffer first (thread-safe)
    snprintf(time_buffer, sizeof(time_buffer), "%02d:%02d %s", display_hours, timeinfo.tm_min, ampm);
    
    // Update the label with current time (single LVGL call)
    lv_label_set_text(time_label, time_buffer);
    printf("UI: Timer updated time display: %s\n", time_buffer);
    
    // Update WiFi icon if status changed (thread-safe)
    update_wifi_icon_display();
}

// Thread-safe function to update WiFi status (called from event handlers)
void set_wifi_icon_status(int status) {
    current_wifi_status = status;
}

// Internal function to actually update the WiFi icon (called from LVGL context only)
static void update_wifi_icon_display(void) {
    if (!wifi_icon || last_displayed_wifi_status == current_wifi_status) return;
    
    if (current_wifi_status == 0) { // Disconnected
        lv_obj_set_style_text_color(wifi_icon, lv_color_hex(0xFF0000), LV_PART_MAIN | LV_STATE_DEFAULT); // Red
    } else if (current_wifi_status == 1) { // Connecting
        lv_obj_set_style_text_color(wifi_icon, lv_color_hex(0xFFFF00), LV_PART_MAIN | LV_STATE_DEFAULT); // Yellow
    } else if (current_wifi_status == 2) { // Connected
        lv_obj_set_style_text_color(wifi_icon, lv_color_hex(0x00FF00), LV_PART_MAIN | LV_STATE_DEFAULT); // Green
    }
    
    last_displayed_wifi_status = current_wifi_status;
}

// Function to create common elements (WiFi icon and time) on any screen
static void create_common_elements(lv_obj_t *screen)
{
    // Create WiFi icon in top left
    wifi_icon = lv_label_create(screen);
    lv_label_set_text(wifi_icon, wifi_symbol);
    lv_obj_set_style_text_font(wifi_icon, &lv_font_montserrat_24, 0);
    lv_obj_align(wifi_icon, LV_ALIGN_TOP_LEFT, 10, 10);
    
    // Apply current WiFi status (maintain last known status when switching screens)
    // Reset the last_displayed_wifi_status to force update with current status
    last_displayed_wifi_status = -1;
    update_wifi_icon_display();  // Apply current stored status
    
    // Create time label in top right
    time_label = lv_label_create(screen);
    lv_obj_set_style_text_color(time_label, lv_color_black(), 0);
    lv_obj_set_style_text_font(time_label, &lv_font_montserrat_24, 0);
    lv_obj_align(time_label, LV_ALIGN_TOP_RIGHT, -10, 10);
    
    // ALWAYS get fresh time when creating any screen to ensure sync
    time_t now;
    struct tm timeinfo;
    static char fresh_time_buffer[32];
    
    // Get current time from RTC (already includes timezone adjustment)
    time_local(&now);
    
    // Use gmtime_r in a thread-safe manner with local copy
    if (gmtime_r(&now, &timeinfo) != NULL) {
        // Convert to 12-hour format
        int display_hours = timeinfo.tm_hour;
        const char* ampm = "AM";
        
        if (timeinfo.tm_hour == 0) {
            display_hours = 12;
        } else if (timeinfo.tm_hour > 12) {
            display_hours = timeinfo.tm_hour - 12;
            ampm = "PM";
        } else if (timeinfo.tm_hour == 12) {
            ampm = "PM";
        }
        
        // Format fresh time string
        snprintf(fresh_time_buffer, sizeof(fresh_time_buffer), "%02d:%02d %s", display_hours, timeinfo.tm_min, ampm);
        
        // Set the fresh time immediately
        lv_label_set_text(time_label, fresh_time_buffer);
        printf("UI: Time updated on screen creation: %s\n", fresh_time_buffer);
    } else {
        printf("UI: Failed to get current time for screen creation\n");
    }
}

// Function to create ads screen
static void create_ads_screen1(const char* ads_data)
{
    if (ads_screen != NULL) {
        lv_obj_del(ads_screen); // Delete if it already exists
    }
    
    // Create ads screen
    ads_screen = lv_obj_create(NULL);
    
    // Set initial background properties (important for color changes)
    lv_obj_set_style_bg_color(ads_screen, lv_color_white(), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(ads_screen, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_clear_flag(ads_screen, LV_OBJ_FLAG_SCROLLABLE); // Disable scrolling
    
    printf("Created ads screen with white background\n");
    
    // Create main ads text in center FIRST (before common elements)
    ads_text_obj = lv_label_create(ads_screen);
    
    // Format the text with ads_data value + 1
    static char ads_text_buffer1[32];
    if (ads_data != NULL) {
        int ads_value = atoi(ads_data);  // Convert string to integer
        snprintf(ads_text_buffer1, sizeof(ads_text_buffer1), "WASHROOM %d", ads_value + 1);
    } else {
        snprintf(ads_text_buffer1, sizeof(ads_text_buffer1), "WASHROOM 1");  // Default fallback
    }
    
    lv_label_set_text(ads_text_obj, ads_text_buffer1);
    lv_obj_set_style_text_color(ads_text_obj, lv_color_make(255, 0, 0), LV_PART_MAIN);  // Red color
    lv_obj_set_style_text_font(ads_text_obj, &lv_font_montserrat_34, LV_PART_MAIN);
    lv_obj_set_style_text_align(ads_text_obj, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);
    lv_obj_align(ads_text_obj, LV_ALIGN_CENTER, 0, -20); // Slightly above center
    
    // Create elapsed time timer label below main text
    ads_timer_label = lv_label_create(ads_screen);
    
    // Format current elapsed time
    uint32_t minutes = ads_elapsed_seconds / 60;
    uint32_t seconds = ads_elapsed_seconds % 60;
    static char elapsed_buffer[16];
    snprintf(elapsed_buffer, sizeof(elapsed_buffer), "%02lu:%02lu", 
             (unsigned long)minutes, (unsigned long)seconds);
    lv_label_set_text(ads_timer_label, elapsed_buffer);
    
    lv_obj_set_style_text_color(ads_timer_label, lv_color_make(255, 0, 0), LV_PART_MAIN);  // Red color (matching text)
    lv_obj_set_style_text_font(ads_timer_label, &lv_font_montserrat_24, LV_PART_MAIN);
    lv_obj_set_style_text_align(ads_timer_label, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);
    lv_obj_align(ads_timer_label, LV_ALIGN_CENTER, 0, 20); // Below main text
    
    printf("Created ads text: %s\n", ads_text_buffer1);
    
    // Add common elements to ads screen AFTER main text
    create_common_elements(ads_screen);
}

static void create_ads_screen2(const char* ads_data)
{
    if (ads_screen != NULL) {
        lv_obj_del(ads_screen); // Delete if it already exists
    }
    
    // Create ads screen
    ads_screen = lv_obj_create(NULL);
    
    // Set initial background properties (important for color changes)
    lv_obj_set_style_bg_color(ads_screen, lv_color_make(255,0,0), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(ads_screen, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_clear_flag(ads_screen, LV_OBJ_FLAG_SCROLLABLE); // Disable scrolling
    
    printf("Created ads screen with red background\n");
    
    // Create main ads text in center FIRST (before common elements)
    ads_text_obj = lv_label_create(ads_screen);
    
    // Format the text with ads_data value + 1
    static char ads_text_buffer2[32];
    if (ads_data != NULL) {
        int ads_value = atoi(ads_data);  // Convert string to integer
        snprintf(ads_text_buffer2, sizeof(ads_text_buffer2), "WASHROOM %d", ads_value + 1);
    } else {
        snprintf(ads_text_buffer2, sizeof(ads_text_buffer2), "WASHROOM 1");  // Default fallback
    }
    
    lv_label_set_text(ads_text_obj, ads_text_buffer2);
    lv_obj_set_style_text_color(ads_text_obj, lv_color_white(), LV_PART_MAIN);  // White color
    lv_obj_set_style_text_font(ads_text_obj, &lv_font_montserrat_34, LV_PART_MAIN);
    lv_obj_set_style_text_align(ads_text_obj, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);
    lv_obj_align(ads_text_obj, LV_ALIGN_CENTER, 0, -20); // Slightly above center
    
    // Create elapsed time timer label below main text
    ads_timer_label = lv_label_create(ads_screen);
    
    // Format current elapsed time
    uint32_t minutes = ads_elapsed_seconds / 60;
    uint32_t seconds = ads_elapsed_seconds % 60;
    static char elapsed_buffer2[16];
    snprintf(elapsed_buffer2, sizeof(elapsed_buffer2), "%02lu:%02lu", 
             (unsigned long)minutes, (unsigned long)seconds);
    lv_label_set_text(ads_timer_label, elapsed_buffer2);
    
    lv_obj_set_style_text_color(ads_timer_label, lv_color_white(), LV_PART_MAIN);  // White color (matching text)
    lv_obj_set_style_text_font(ads_timer_label, &lv_font_montserrat_24, LV_PART_MAIN);
    lv_obj_set_style_text_align(ads_timer_label, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);
    lv_obj_align(ads_timer_label, LV_ALIGN_CENTER, 0, 20); // Below main text
    
    printf("Created ads text: %s\n", ads_text_buffer2);
    
    // Add common elements to ads screen AFTER main text
    create_common_elements(ads_screen);
}

// Function to switch to ads screen (thread-safe)
void switch_to_ads_screen(const char* ads_data)
{
    if (!display_ref) return;
    
    create_ads_screen(ads_data);
    lv_scr_load(ads_screen);
    current_screen = SCREEN_ADS;
}

// Function to switch back to alert screen (thread-safe)
void switch_to_alert_screen(void)
{
    if (!display_ref || !alert_screen) return;
    
    // Stop ads timer when switching away from ads screen
    if (current_screen == SCREEN_ADS) {
        if (ads_switch_timer) {
            lv_timer_del(ads_switch_timer);
            ads_switch_timer = NULL;
            printf("Stopped ads switching timer\n");
        }
        
        // Free stored ads data
        if (current_ads_data) {
            free(current_ads_data);
            current_ads_data = NULL;
        }
        
        // Reset elapsed time counter
        ads_elapsed_seconds = 0;
    }
    
    lv_scr_load(alert_screen);
    current_screen = SCREEN_ALERT;
    
    // Common elements are already created when alert_screen was initialized
    // Just update the references and status when switching back
    // The timer will handle time updates automatically
}


void alert_ui_init(lv_display_t *disp)
{
    // Store display reference for screen switching
    display_ref = disp;
    
    // Get the active screen (this becomes our alert screen)
    alert_screen = lv_display_get_screen_active(disp);
    
    // Set screen background to white
    lv_obj_set_style_bg_color(alert_screen, lv_color_white(), 0);
    lv_obj_set_style_bg_opa(alert_screen, LV_OPA_COVER, 0);
    
    // Add common elements to alert screen
    create_common_elements(alert_screen);
    
    //Create Image in the center
    LV_IMG_DECLARE(logo); // Declare the image (make sure logo.c is included in the project)
    lv_obj_t * img = lv_img_create(alert_screen);
    lv_img_set_src(img, &logo);
    lv_obj_align(img, LV_ALIGN_CENTER, 0, -50); // Position image above center

    // Create alert text below the image
    alert_text_obj = lv_label_create(alert_screen);
    lv_label_set_text(alert_text_obj, "TOILET CLEANING ALERT");
    lv_obj_set_style_text_color(alert_text_obj, lv_color_make(255, 165, 0), 0);  // Initial orange color
    lv_obj_set_style_text_font(alert_text_obj, &lv_font_montserrat_34, 0);
    lv_obj_set_style_text_align(alert_text_obj, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_align(alert_text_obj, LV_ALIGN_CENTER, 0, 50); // Position text below center
    
    // Hue animation (orange range)
    lv_anim_t hue_anim;
    lv_anim_init(&hue_anim);
    lv_anim_set_var(&hue_anim, alert_text_obj);
    lv_anim_set_values(&hue_anim, 20, 40);
    lv_anim_set_exec_cb(&hue_anim, hue_anim_cb);
    lv_anim_set_duration(&hue_anim, 1500);
    lv_anim_set_repeat_count(&hue_anim, LV_ANIM_REPEAT_INFINITE);
    lv_anim_start(&hue_anim);

    // Create a timer to update time and WiFi status (every 1 minute)
    lv_timer_create(update_time_label, 10000, NULL);
    
    // Initial time update
    update_time_label(NULL);
}
