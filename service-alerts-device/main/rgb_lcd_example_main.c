/*
 * SPDX-FileCopyrightText: 2022-2024 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: CC0-1.0
 */

#include <stdio.h>
#include <unistd.h>
#include <sys/lock.h>
#include <string.h>
#include <stdlib.h>
#include "cJSON.h"
#include "app_iotcore.h"
#if CONFIG_ENABLE_NVS
#include "nvs_read_write.h"
#endif
#include "sdkconfig.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_timer.h"
#include "esp_lcd_panel_ops.h"
#include "esp_lcd_panel_rgb.h"
#include "driver/gpio.h"
#include "esp_err.h"
#include "esp_log.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "wifi_manager.h"
#include "esp_heap_caps.h"
#include "lvgl.h"
#include "ui.h"
#include "mqtt_data_model.h"
#include "app_rtc.h"
#include "esp_sntp.h"


static const char *TAG = "main";

// Global variables
static int32_t ID = 0;

// Function declarations
void PUB_SUB(int32_t client_id);
static void wifi_event_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data);

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////// Please update the following configuration according to your LCD spec //////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Refresh Rate = 22000000/(96+48+16+640)/(2+33+10+480) = 60Hz
#define EXAMPLE_LCD_PIXEL_CLOCK_HZ     (21 * 1000 * 1000)
#define EXAMPLE_LCD_H_RES              640
#define EXAMPLE_LCD_V_RES              480
#define EXAMPLE_LCD_HSYNC              96
#define EXAMPLE_LCD_HBP                48
#define EXAMPLE_LCD_HFP                16
#define EXAMPLE_LCD_VSYNC              2
#define EXAMPLE_LCD_VBP                33
#define EXAMPLE_LCD_VFP                10

#define EXAMPLE_LCD_BK_LIGHT_ON_LEVEL  1
#define EXAMPLE_LCD_BK_LIGHT_OFF_LEVEL !EXAMPLE_LCD_BK_LIGHT_ON_LEVEL
#define EXAMPLE_PIN_NUM_BK_LIGHT       -1
#define EXAMPLE_PIN_NUM_DISP_EN        -1

#define EXAMPLE_PIN_NUM_HSYNC          CONFIG_EXAMPLE_LCD_HSYNC_GPIO
#define EXAMPLE_PIN_NUM_VSYNC          CONFIG_EXAMPLE_LCD_VSYNC_GPIO
#define EXAMPLE_PIN_NUM_DE             CONFIG_EXAMPLE_LCD_DE_GPIO
#define EXAMPLE_PIN_NUM_PCLK           CONFIG_EXAMPLE_LCD_PCLK_GPIO

#define EXAMPLE_PIN_NUM_DATA0          CONFIG_EXAMPLE_LCD_DATA0_GPIO
#define EXAMPLE_PIN_NUM_DATA1          CONFIG_EXAMPLE_LCD_DATA1_GPIO
#define EXAMPLE_PIN_NUM_DATA2          CONFIG_EXAMPLE_LCD_DATA2_GPIO
#define EXAMPLE_PIN_NUM_DATA3          CONFIG_EXAMPLE_LCD_DATA3_GPIO
#define EXAMPLE_PIN_NUM_DATA4          CONFIG_EXAMPLE_LCD_DATA4_GPIO
#define EXAMPLE_PIN_NUM_DATA5          CONFIG_EXAMPLE_LCD_DATA5_GPIO
#define EXAMPLE_PIN_NUM_DATA6          CONFIG_EXAMPLE_LCD_DATA6_GPIO
#define EXAMPLE_PIN_NUM_DATA7          CONFIG_EXAMPLE_LCD_DATA7_GPIO
#define EXAMPLE_PIN_NUM_DATA8          CONFIG_EXAMPLE_LCD_DATA8_GPIO
#define EXAMPLE_PIN_NUM_DATA9          CONFIG_EXAMPLE_LCD_DATA9_GPIO
#define EXAMPLE_PIN_NUM_DATA10         CONFIG_EXAMPLE_LCD_DATA10_GPIO
#define EXAMPLE_PIN_NUM_DATA11         CONFIG_EXAMPLE_LCD_DATA11_GPIO
#define EXAMPLE_PIN_NUM_DATA12         CONFIG_EXAMPLE_LCD_DATA12_GPIO
#define EXAMPLE_PIN_NUM_DATA13         CONFIG_EXAMPLE_LCD_DATA13_GPIO
#define EXAMPLE_PIN_NUM_DATA14         CONFIG_EXAMPLE_LCD_DATA14_GPIO
#define EXAMPLE_PIN_NUM_DATA15         CONFIG_EXAMPLE_LCD_DATA15_GPIO
#if CONFIG_EXAMPLE_LCD_DATA_LINES > 16
#define EXAMPLE_PIN_NUM_DATA16         CONFIG_EXAMPLE_LCD_DATA16_GPIO
#define EXAMPLE_PIN_NUM_DATA17         CONFIG_EXAMPLE_LCD_DATA17_GPIO
#define EXAMPLE_PIN_NUM_DATA18         CONFIG_EXAMPLE_LCD_DATA18_GPIO
#define EXAMPLE_PIN_NUM_DATA19         CONFIG_EXAMPLE_LCD_DATA19_GPIO
#define EXAMPLE_PIN_NUM_DATA20         CONFIG_EXAMPLE_LCD_DATA20_GPIO
#define EXAMPLE_PIN_NUM_DATA21         CONFIG_EXAMPLE_LCD_DATA21_GPIO
#define EXAMPLE_PIN_NUM_DATA22         CONFIG_EXAMPLE_LCD_DATA22_GPIO
#define EXAMPLE_PIN_NUM_DATA23         CONFIG_EXAMPLE_LCD_DATA23_GPIO
#endif

#if CONFIG_EXAMPLE_USE_DOUBLE_FB
#define EXAMPLE_LCD_NUM_FB             2
#else
#define EXAMPLE_LCD_NUM_FB             1
#endif // CONFIG_EXAMPLE_USE_DOUBLE_FB

#if CONFIG_EXAMPLE_LCD_DATA_LINES_16
#define EXAMPLE_DATA_BUS_WIDTH         16
#define EXAMPLE_PIXEL_SIZE             2
#define EXAMPLE_LV_COLOR_FORMAT        LV_COLOR_FORMAT_RGB565
#elif CONFIG_EXAMPLE_LCD_DATA_LINES_24
#define EXAMPLE_DATA_BUS_WIDTH         24
#define EXAMPLE_PIXEL_SIZE             3
#define EXAMPLE_LV_COLOR_FORMAT        LV_COLOR_FORMAT_RGB888
#endif

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////// Please update the following configuration according to your Application ///////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#define EXAMPLE_LVGL_DRAW_BUF_LINES    480 // number of display lines in each draw buffer
#define EXAMPLE_LVGL_TICK_PERIOD_MS    2
#define EXAMPLE_LVGL_TASK_STACK_SIZE   (5 * 1024)
#define EXAMPLE_LVGL_TASK_PRIORITY     2

// LVGL library is not thread-safe, this example will call LVGL APIs from different tasks, so use a mutex to protect it
static _lock_t lvgl_api_lock;

// Forward declaration of the Alert UI function
extern void alert_ui_init(lv_display_t *disp);

static bool example_notify_lvgl_flush_ready(esp_lcd_panel_handle_t panel, const esp_lcd_rgb_panel_event_data_t *event_data, void *user_ctx)
{
    lv_display_t *disp = (lv_display_t *)user_ctx;
    lv_display_flush_ready(disp);
    return false;
}

static void example_lvgl_flush_cb(lv_display_t *disp, const lv_area_t *area, uint8_t *px_map)
{
    esp_lcd_panel_handle_t panel_handle = lv_display_get_user_data(disp);
    int offsetx1 = area->x1;
    int offsetx2 = area->x2;
    int offsety1 = area->y1;
    int offsety2 = area->y2;
    // pass the draw buffer to the driver
    esp_lcd_panel_draw_bitmap(panel_handle, offsetx1, offsety1, offsetx2 + 1, offsety2 + 1, px_map);
}

static void example_increase_lvgl_tick(void *arg)
{
    /* Tell LVGL how many milliseconds has elapsed */
    lv_tick_inc(EXAMPLE_LVGL_TICK_PERIOD_MS);
}

static void example_lvgl_port_task(void *arg)
{
    ESP_LOGI(TAG, "Starting LVGL task");
    uint32_t time_till_next_ms = 0;
    while (1) {
        _lock_acquire(&lvgl_api_lock);
        time_till_next_ms = lv_timer_handler();
        _lock_release(&lvgl_api_lock);

        // in case of task watch dog timeout, set the minimal delay to 10ms
        if (time_till_next_ms < 10) {
            time_till_next_ms = 10;
        }

        usleep(1000 * time_till_next_ms);
    }
}

static void example_bsp_init_lcd_backlight(void)
{
#if EXAMPLE_PIN_NUM_BK_LIGHT >= 0
    gpio_config_t bk_gpio_config = {
        .mode = GPIO_MODE_OUTPUT,
        .pin_bit_mask = 1ULL << EXAMPLE_PIN_NUM_BK_LIGHT
    };
    ESP_ERROR_CHECK(gpio_config(&bk_gpio_config));
#endif
}

static void example_bsp_set_lcd_backlight(uint32_t level)
{
#if EXAMPLE_PIN_NUM_BK_LIGHT >= 0
    gpio_set_level(EXAMPLE_PIN_NUM_BK_LIGHT, level);
#endif
}


// Add a global variable to track WiFi status
static int last_wifi_status = 0; // 0: disconnected, 1: connecting, 2: connected

static void wifi_event_handler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data)
{
    if (event_base == WIFI_EVENT) {
        switch (event_id) {
            case WIFI_EVENT_STA_START:
                set_wifi_icon_status(1); // Connecting (yellow)
                last_wifi_status = 1;
                break;
            case WIFI_EVENT_STA_CONNECTED:
                set_wifi_icon_status(2); // Connected (green)
                last_wifi_status = 2;
                break;
            case WIFI_EVENT_STA_DISCONNECTED:
                set_wifi_icon_status(0); // Disconnected (red)
                last_wifi_status = 0;
                break;
            default:
                break;
        }
    }
}


// --- MQTT Message Handler ---
void mqtt_message_handler(const char* topic, uint16_t topic_len, const char* message, uint16_t message_len)
{
    // Handle alert/ads topic - switch to ads screen
    ESP_LOGI(TAG, "Received ads message: %.*s", message_len, message);
    
    // Create null-terminated string for the message
    char* ads_message = malloc(message_len + 1);
    if (ads_message) {
        memcpy(ads_message, message, message_len);
        ads_message[message_len] = '\0';
        
        // Switch to ads screen with the message
        _lock_acquire(&lvgl_api_lock);
        switch_to_ads_screen(ads_message);
        _lock_release(&lvgl_api_lock);
        
        free(ads_message);
    }
}

void mqtt_watchok_message_handler(const char* topic, uint16_t topic_len, const char* message, uint16_t message_len)
{
    // Handle alert/watchok topic - check if message is "1" to switch back
    ESP_LOGI(TAG, "Received watchok message: %.*s", message_len, message);
    
    // Check if message is "1"
    if (message_len == 1 && message[0] == '1') {
        // Switch back to alert screen
        _lock_acquire(&lvgl_api_lock);
        switch_to_alert_screen();
        _lock_release(&lvgl_api_lock);
    }
}



// --- MQTT Event Handler ---
void mqtt_event_handler(const char* event_name, void* event_data)
{
    static uint32_t connection_count = 0;
    static uint32_t disconnection_count = 0;

    if (strcmp(event_name, "MQTT_EVENT_CONNECTED") == 0) {
        connection_count++;
        ESP_LOGI(TAG, "MQTT Connected successfully (Connection #%ld)", connection_count);
        ESP_LOGI(TAG, "MQTT Connection stats - Connects: %ld, Disconnects: %ld", connection_count, disconnection_count);
        PUB_SUB(ID); // Always re-subscribe on connect
    } else if (strcmp(event_name, "MQTT_EVENT_DISCONNECTED") == 0) {
        disconnection_count++;
        ESP_LOGW(TAG, "MQTT Disconnected (Disconnection #%ld) - Will attempt reconnection", disconnection_count);
        ESP_LOGI(TAG, "MQTT Connection stats - Connects: %ld, Disconnects: %ld", connection_count, disconnection_count);
    } else if (strcmp(event_name, "MQTT_EVENT_ERROR") == 0) {
        ESP_LOGE(TAG, "MQTT Error occurred");
    } else {
        ESP_LOGI(TAG, "MQTT Event: %s", event_name);
    }
}



// PUB_SUB function implementation
// --- MQTT Subscribe ---
void PUB_SUB(int32_t clientID)
{
    static bool already_subscribed = false;
    if (already_subscribed) return;

    char *topic = NULL;
    asprintf(&topic, "alert/ads");
    post_mqtt_subscribe_event(topic, 0, mqtt_message_handler);
    asprintf(&topic, "alert/watchok");
    post_mqtt_subscribe_event(topic, 0, mqtt_watchok_message_handler);
    ESP_LOGI(TAG, "Subscribed to button topic: %s", topic);
    already_subscribed = true;
    free(topic);
}

// RTC synchronization task - Checks NTP sync every 5 minutes
static void rtc_sync_task(void *pvParameters)
{
    static const char* TAG_SYNC = "RTC_SYNC_TASK";
    static const uint32_t SYNC_CHECK_INTERVAL_MS = 300000; // Check every 5 minutes (300 seconds)
    static const uint32_t INITIAL_DELAY_MS = 30000; // Wait 30 seconds after boot
    
    ESP_LOGI(TAG_SYNC, "RTC sync task started");
    
    // Initial delay to allow system to stabilize
    vTaskDelay(pdMS_TO_TICKS(INITIAL_DELAY_MS));
    
    while (1) {
        ESP_LOGI(TAG_SYNC, "Performing periodic RTC-NTP sync check (every 5 minutes)");
        
        time_t now;
        time_local(&now);
        
        // Check if NTP sync event bit is set
        if (rtc_event_group) {
            EventBits_t sync_bits = xEventGroupGetBits(rtc_event_group);
            
            if (!(sync_bits & TIME_SYNC_BIT)) {
                ESP_LOGW(TAG_SYNC, "NTP time sync bit not set, attempting to re-sync");
                
                // Clear the sync bit and attempt to re-initialize SNTP
                xEventGroupClearBits(rtc_event_group, TIME_SYNC_BIT);
                
                // Check if SNTP is already initialized
                if (esp_sntp_enabled()) {
                    ESP_LOGI(TAG_SYNC, "SNTP already enabled, restarting sync");
                    esp_sntp_restart();
                } else {
                    ESP_LOGI(TAG_SYNC, "Initializing SNTP for time sync");
                    esp_sntp_setoperatingmode(SNTP_OPMODE_POLL);
                    esp_sntp_setservername(0, "time.windows.com");
                    esp_sntp_setservername(1, "time.google.com");
                    esp_sntp_setservername(2, "pool.ntp.org");
                    esp_sntp_init();
                }
                
                // Wait for sync with timeout (30 seconds)
                EventBits_t bits = xEventGroupWaitBits(
                    rtc_event_group, 
                    TIME_SYNC_BIT, 
                    pdFALSE,  // Don't clear the bit
                    pdFALSE,  // Wait for any bit
                    pdMS_TO_TICKS(30000)  // 30 second timeout
                );
                
                if (bits & TIME_SYNC_BIT) {
                    ESP_LOGI(TAG_SYNC, "NTP time sync successful");
                } else {
                    ESP_LOGW(TAG_SYNC, "NTP time sync timed out");
                }
            } else {
                // Sync bit is set, check if time is reasonable
                struct tm timeinfo;
                time_local(&now);
                gmtime_r(&now, &timeinfo);
                
                // Check if time is reasonable (year > 2020)
                if (timeinfo.tm_year < (2020 - 1900)) {
                    ESP_LOGW(TAG_SYNC, "System time appears invalid (year: %d), forcing re-sync", 
                             timeinfo.tm_year + 1900);
                    
                    // Force a re-sync
                    xEventGroupClearBits(rtc_event_group, TIME_SYNC_BIT);
                    if (esp_sntp_enabled()) {
                        esp_sntp_restart();
                    }
                } else {
                    ESP_LOGD(TAG_SYNC, "RTC time appears to be in sync with NTP (year: %d)", 
                             timeinfo.tm_year + 1900);
                }
            }
        } else {
            ESP_LOGW(TAG_SYNC, "RTC event group not available");
        }
        
        // Wait for the next sync check interval
        vTaskDelay(pdMS_TO_TICKS(SYNC_CHECK_INTERVAL_MS));
    }
}

void app_main(void)
{
    ESP_LOGE(TAG, "Starting application..Available: %ld bytes", esp_get_free_heap_size());
    init_iotcore(NULL);
    ESP_LOGE(TAG, "IOTCORE INIT..Available: %ld bytes", esp_get_free_heap_size());
   
    #if CONFIG_ENABLE_NVS
        readKeyValueInFlash_int32("clientId", &ID);
        ESP_LOGI(TAG, "ID = %" PRId32, ID);
        PUB_SUB(ID);
    #endif

        esp_event_handler_instance_t instance_any_id;
        esp_event_handler_instance_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_event_handler, NULL, &instance_any_id);

    ESP_LOGI(TAG, "Turn off LCD backlight");
    example_bsp_init_lcd_backlight();
    example_bsp_set_lcd_backlight(EXAMPLE_LCD_BK_LIGHT_OFF_LEVEL);

    ESP_LOGI(TAG, "Install RGB LCD panel driver");
    esp_lcd_panel_handle_t panel_handle = NULL;
    esp_lcd_rgb_panel_config_t panel_config = {
        .data_width = EXAMPLE_DATA_BUS_WIDTH,
        .dma_burst_size = 64, 
        .num_fbs = EXAMPLE_LCD_NUM_FB,
#if CONFIG_EXAMPLE_USE_BOUNCE_BUFFER
        .bounce_buffer_size_px = 20 * EXAMPLE_LCD_H_RES,
#endif
        .clk_src = LCD_CLK_SRC_DEFAULT,
        .disp_gpio_num = EXAMPLE_PIN_NUM_DISP_EN,
        .pclk_gpio_num = EXAMPLE_PIN_NUM_PCLK,
        .vsync_gpio_num = EXAMPLE_PIN_NUM_VSYNC,
        .hsync_gpio_num = EXAMPLE_PIN_NUM_HSYNC,
        .de_gpio_num = EXAMPLE_PIN_NUM_DE,
        .data_gpio_nums = {
            EXAMPLE_PIN_NUM_DATA0,
            EXAMPLE_PIN_NUM_DATA1,
            EXAMPLE_PIN_NUM_DATA2,
            EXAMPLE_PIN_NUM_DATA3,
            EXAMPLE_PIN_NUM_DATA4,
            EXAMPLE_PIN_NUM_DATA5,
            EXAMPLE_PIN_NUM_DATA6,
            EXAMPLE_PIN_NUM_DATA7,
            EXAMPLE_PIN_NUM_DATA8,
            EXAMPLE_PIN_NUM_DATA9,
            EXAMPLE_PIN_NUM_DATA10,
            EXAMPLE_PIN_NUM_DATA11,
            EXAMPLE_PIN_NUM_DATA12,
            EXAMPLE_PIN_NUM_DATA13,
            EXAMPLE_PIN_NUM_DATA14,
            EXAMPLE_PIN_NUM_DATA15,
#if CONFIG_EXAMPLE_LCD_DATA_LINES > 16
            EXAMPLE_PIN_NUM_DATA16,
            EXAMPLE_PIN_NUM_DATA17,
            EXAMPLE_PIN_NUM_DATA18,
            EXAMPLE_PIN_NUM_DATA19,
            EXAMPLE_PIN_NUM_DATA20,
            EXAMPLE_PIN_NUM_DATA21,
            EXAMPLE_PIN_NUM_DATA22,
            EXAMPLE_PIN_NUM_DATA23
#endif
        },
        .timings = {
            .pclk_hz = EXAMPLE_LCD_PIXEL_CLOCK_HZ,
            .h_res = EXAMPLE_LCD_H_RES,
            .v_res = EXAMPLE_LCD_V_RES,
            .hsync_back_porch = EXAMPLE_LCD_HBP,
            .hsync_front_porch = EXAMPLE_LCD_HFP,
            .hsync_pulse_width = EXAMPLE_LCD_HSYNC,
            .vsync_back_porch = EXAMPLE_LCD_VBP,
            .vsync_front_porch = EXAMPLE_LCD_VFP,
            .vsync_pulse_width = EXAMPLE_LCD_VSYNC,
            .flags = {
                .pclk_active_neg = true,
            },
        },
        .flags.fb_in_psram = true, // allocate frame buffer in PSRAM
    };
    ESP_ERROR_CHECK(esp_lcd_new_rgb_panel(&panel_config, &panel_handle));

    ESP_LOGI(TAG, "Initialize RGB LCD panel");
    ESP_ERROR_CHECK(esp_lcd_panel_reset(panel_handle));
    ESP_ERROR_CHECK(esp_lcd_panel_init(panel_handle));

    ESP_LOGI(TAG, "Turn on LCD backlight");
    example_bsp_set_lcd_backlight(EXAMPLE_LCD_BK_LIGHT_ON_LEVEL);

    ESP_LOGI(TAG, "Initialize LVGL library");
    lv_init();
    // create a lvgl display
    lv_display_t *display = lv_display_create(EXAMPLE_LCD_H_RES, EXAMPLE_LCD_V_RES);
    // associate the rgb panel handle to the display
    lv_display_set_user_data(display, panel_handle);
    // set color depth
    lv_display_set_color_format(display, EXAMPLE_LV_COLOR_FORMAT);
    // create draw buffers
    void *buf1 = NULL;
    void *buf2 = NULL;
#if CONFIG_EXAMPLE_USE_DOUBLE_FB
    ESP_LOGI(TAG, "Use frame buffers as LVGL draw buffers");
    ESP_ERROR_CHECK(esp_lcd_rgb_panel_get_frame_buffer(panel_handle, 2, &buf1, &buf2));
    // set LVGL draw buffers and direct mode
    // lv_display_set_buffers(display, buf1, buf2, EXAMPLE_LCD_H_RES * EXAMPLE_LCD_V_RES * EXAMPLE_PIXEL_SIZE, LV_DISPLAY_RENDER_MODE_DIRECT);
    lv_display_set_buffers(display, buf1, buf2, EXAMPLE_LCD_H_RES * EXAMPLE_LCD_V_RES * EXAMPLE_PIXEL_SIZE, LV_DISPLAY_RENDER_MODE_FULL);

#else
    ESP_LOGI(TAG, "Allocate LVGL draw buffers");
    // it's recommended to allocate the draw buffer from internal memory, for better performance
    size_t draw_buffer_sz = EXAMPLE_LCD_H_RES * EXAMPLE_LVGL_DRAW_BUF_LINES * EXAMPLE_PIXEL_SIZE;
    // buf1 = heap_caps_malloc(draw_buffer_sz, MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);
    buf1 = heap_caps_malloc(draw_buffer_sz, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
    if (buf1 == NULL) {
    ESP_LOGE(TAG, "Failed to allocate draw buffer in PSRAM, size=%zu", draw_buffer_sz);
    abort();
    }
    else{
        assert(buf1);
    }
    ESP_LOGE(TAG, "BUFF 1..Available: %ld bytes", esp_get_free_heap_size());
    // set LVGL draw buffers and partial mode
    lv_display_set_buffers(display, buf1, buf2, draw_buffer_sz, LV_DISPLAY_RENDER_MODE_PARTIAL);
#endif // CONFIG_EXAMPLE_USE_DOUBLE_FB

    // set the callback which can copy the rendered image to an area of the display
    lv_display_set_flush_cb(display, example_lvgl_flush_cb);

    ESP_LOGI(TAG, "Register event callbacks");
    esp_lcd_rgb_panel_event_callbacks_t cbs = {
        .on_color_trans_done = example_notify_lvgl_flush_ready,
    };
    ESP_ERROR_CHECK(esp_lcd_rgb_panel_register_event_callbacks(panel_handle, &cbs, display));

    ESP_LOGI(TAG, "Install LVGL tick timer");
    // Tick interface for LVGL (using esp_timer to generate 2ms periodic event)
    const esp_timer_create_args_t lvgl_tick_timer_args = {
        .callback = &example_increase_lvgl_tick,
        .name = "lvgl_tick"
    };
    esp_timer_handle_t lvgl_tick_timer = NULL;
    ESP_ERROR_CHECK(esp_timer_create(&lvgl_tick_timer_args, &lvgl_tick_timer));
    ESP_ERROR_CHECK(esp_timer_start_periodic(lvgl_tick_timer, EXAMPLE_LVGL_TICK_PERIOD_MS * 1000));

    ESP_LOGI(TAG, "Create LVGL task");
    xTaskCreate(example_lvgl_port_task, "LVGL", EXAMPLE_LVGL_TASK_STACK_SIZE, NULL, EXAMPLE_LVGL_TASK_PRIORITY, NULL);

    ESP_LOGI(TAG, "Create RTC sync task");
    xTaskCreate(rtc_sync_task, "RTC_SYNC", 4096, NULL, 3, NULL);

    ESP_LOGI(TAG, "Display LVGL UI");
    // Lock the mutex due to the LVGL APIs are not thread-safe
    _lock_acquire(&lvgl_api_lock);
    alert_ui_init(display);
    _lock_release(&lvgl_api_lock);
}
