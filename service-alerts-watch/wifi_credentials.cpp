#include "wifi_credentials.h"
#include <Preferences.h>
#include <WiFi.h>
#include <lvgl.h>
#include "ui.h" // adjust if your UI label is defined elsewhere
#include <esp_wifi.h>

// RTC sync globals (declare as extern, define elsewhere)
extern bool rtc_synced;
extern void simple_rtc_sync();

// Expose wifi_cred_state for use in .ino
WifiCredentialState wifi_cred_state;


Preferences wifiPrefs;

void save_wifi_credential(const char* ssid, const char* password) {
    Serial.printf("[DEBUG] save_wifi_credential: Attempting to save SSID='%s'\n", ssid);
    wifiPrefs.begin(WIFI_NAMESPACE, false);
    // Check if already stored
    for (int i = 0; i < MAX_WIFI_CREDENTIALS; i++) {
        String key_ssid = String("ssid_") + i;
        String key_pass = String("pass_") + i;
        String stored_ssid = wifiPrefs.getString(key_ssid.c_str(), "");
        if (stored_ssid == ssid) {
            // Update password if needed
            wifiPrefs.putString(key_pass.c_str(), password);
            Serial.printf("[DEBUG] save_wifi_credential: Updated password for SSID='%s' at slot %d\n", ssid, i);
            wifiPrefs.end();
            return;
        }
    }
    // Find empty slot
    for (int i = 0; i < MAX_WIFI_CREDENTIALS; i++) {
        String key_ssid = String("ssid_") + i;
        String stored_ssid = wifiPrefs.getString(key_ssid.c_str(), "");
        if (stored_ssid == "") {
            wifiPrefs.putString(key_ssid.c_str(), ssid);
            wifiPrefs.putString((String("pass_") + i).c_str(), password);
            Serial.printf("[DEBUG] save_wifi_credential: Saved SSID='%s' at slot %d\n", ssid, i);
            wifiPrefs.end();
            return;
        }
    }
    Serial.println("[DEBUG] save_wifi_credential: No empty slot found, not saved.");
    wifiPrefs.end();
}

int load_wifi_credentials(WifiCredential creds[MAX_WIFI_CREDENTIALS]) {
    Serial.println("[DEBUG] load_wifi_credentials: Loading credentials from NVS...");
    wifiPrefs.begin(WIFI_NAMESPACE, true);
    int count = 0;
        for (int i = 0; i < MAX_WIFI_CREDENTIALS; i++) {
            String key_ssid = String("ssid_") + i;
            String key_pass = String("pass_") + i;
            String ssid = wifiPrefs.getString(key_ssid.c_str(), "");
            String password = wifiPrefs.getString(key_pass.c_str(), "");
            ssid.trim();
            password.trim();
            if (ssid.length() > 0) {
                creds[count].ssid = ssid;
                creds[count].password = password;
                Serial.printf("[DEBUG] load_wifi_credentials: Found SSID='|%s|' (len=%d) PASS='|%s|' (len=%d) at slot %d\n", ssid.c_str(), ssid.length(), password.c_str(), password.length(), i);
                count++;
            }
        key_ssid = String("ssid_") + i;
        key_pass = String("pass_") + i;
        ssid = wifiPrefs.getString(key_ssid.c_str(), "");
        password = wifiPrefs.getString(key_pass.c_str(), "");
        if (ssid != "") {
            creds[count].ssid = ssid;
            creds[count].password = password;
            Serial.printf("[DEBUG] load_wifi_credentials: Found SSID='%s' at slot %d\n", ssid.c_str(), i);
            count++;
        }
    }
    Serial.printf("[DEBUG] load_wifi_credentials: Total loaded = %d\n", count);
    wifiPrefs.end();
    return count;
}

void clear_wifi_credentials() {
    wifiPrefs.begin(WIFI_NAMESPACE, false);
    wifiPrefs.clear();
    wifiPrefs.end();
}




extern "C" void start_try_connect_all_stored_wifi() {
    wifi_cred_state.count = load_wifi_credentials(wifi_cred_state.creds);
    wifi_cred_state.current = 0;
    wifi_cred_state.in_progress = (wifi_cred_state.count > 0);
        // return false;

}

// Call this repeatedly from main loop or check_wifi_status()
// Returns true if connected, false if still trying or all failed
bool process_try_connect_all_stored_wifi() {
    if (!wifi_cred_state.in_progress || wifi_cred_state.count == 0) return false;

    if (WiFi.status() == WL_CONNECTED) {
        wifi_cred_state.in_progress = false;
        return true;
    }

    // If not already trying, start connection attempt
    if (wifi_cred_state.attempt_start == 0) {
        WiFi.disconnect(true);
        delay(50);
        WiFi.begin(wifi_cred_state.creds[wifi_cred_state.current].ssid.c_str(),
                   wifi_cred_state.creds[wifi_cred_state.current].password.c_str());
        wifi_cred_state.attempt_start = millis();
        Serial.printf("[WIFI] Trying stored credential %d: SSID='%s'\n", wifi_cred_state.current, wifi_cred_state.creds[wifi_cred_state.current].ssid.c_str());
    }

    // Wait up to 5 seconds for this credential
    if (millis() - wifi_cred_state.attempt_start > 5000) {
        Serial.println("[WIFI] Timeout, moving to next stored credential.");
        wifi_cred_state.current++;
        wifi_cred_state.attempt_start = 0;
        if (wifi_cred_state.current >= wifi_cred_state.count) {
            wifi_cred_state.in_progress = false;
            return false;
        }
    }
    return false;
}

// WiFi connection function
void connect_to_wifi(const char* ssid, const char* password) {
    // Double-check for empty SSID (should be caught earlier, but just to be safe)
    if (strlen(ssid) == 0) {
        lv_label_set_text(ui_StatusLabel, "Error: SSID cannot be empty");
        lv_obj_set_style_text_color(ui_StatusLabel, lv_color_hex(0xFF0000), LV_PART_MAIN | LV_STATE_DEFAULT);
        // Make sure status label is visible - force unhide
        lv_obj_clear_flag(ui_StatusLabel, LV_OBJ_FLAG_HIDDEN);
        return;
    }

    // Track user-entered WiFi retry count
    static int user_wifi_retry_count = 0;
    user_wifi_retry_count = 0;
    // Store the user credentials for retry
    static char last_user_ssid[64] = "";
    static char last_user_password[64] = "";
    strncpy(last_user_ssid, ssid, sizeof(last_user_ssid)-1);
    strncpy(last_user_password, password, sizeof(last_user_password)-1);
    last_user_ssid[sizeof(last_user_ssid)-1] = '\0';
    last_user_password[sizeof(last_user_password)-1] = '\0';

    // Start first attempt (non-blocking)
    lv_obj_clear_flag(ui_StatusLabel, LV_OBJ_FLAG_HIDDEN);
    lv_label_set_text(ui_StatusLabel, "Connecting...");
    lv_obj_set_style_text_color(ui_StatusLabel, lv_color_hex(0xFFAA00), LV_PART_MAIN | LV_STATE_DEFAULT);
    Serial.println("Status label should now be visible with 'Connecting...' text");
    // Set up for non-blocking connect in check_wifi_status
    g_user_wifi_pending_disconnect = true;
    wifi_connecting = true;
    wifi_start_time = millis();

    // Wait for connection or timeout (non-blocking handled in check_wifi_status)
    // The retry logic will be handled in check_wifi_status

    // Store retry state globally for check_wifi_status
    g_user_wifi_retry_count = 1;
    strncpy(g_last_user_ssid, last_user_ssid, sizeof(g_last_user_ssid)-1);
    strncpy(g_last_user_password, last_user_password, sizeof(g_last_user_password)-1);
    g_last_user_ssid[sizeof(g_last_user_ssid)-1] = '\0';
    g_last_user_password[sizeof(g_last_user_password)-1] = '\0';
}

void check_wifi_status() {
    // Simple, non-blocking WiFi status check and auto-advance to next stored WiFi if not connected
    if (!wifi_connecting) return;

    // Show connecting status
    static unsigned long last_dot_time = 0;
    static int dots = 0;
    if (millis() - last_dot_time > 500) {
        last_dot_time = millis();
        dots = (dots + 1) % 4;
        String connecting_text = "Connecting";
        for (int i = 0; i < dots; i++) connecting_text += ".";
        lv_label_set_text(ui_StatusLabel, connecting_text.c_str());
    }

    static unsigned long last_disconnect_time = 0;
    wl_status_t status = WiFi.status();
    // Handle non-blocking disconnect/reconnect for user WiFi
    if (g_user_wifi_pending_disconnect) {
        WiFi.disconnect(true);
        g_user_wifi_pending_disconnect = false;
        last_disconnect_time = millis();
        // return; // Wait until next loop to begin
    }
    static bool g_user_wifi_pending_begin = false;
    if (!g_user_wifi_pending_begin && strlen(g_last_user_ssid) > 0 && (millis() - last_disconnect_time > 100)) {
        WiFi.mode(WIFI_STA);
        Serial.printf("Connecting to WiFi SSID: %s (length: %d)\n", g_last_user_ssid, strlen(g_last_user_ssid));
        Serial.printf("Password provided: %s (length: %d)\n", g_last_user_password[0] ? "Yes" : "No", strlen(g_last_user_password));
        WiFi.begin(g_last_user_ssid, g_last_user_password);
        g_user_wifi_pending_begin = true;
        wifi_start_time = millis();
        // return;
    }
    if (status == WL_CONNECTED) {
        // Connected!
        WiFi.setSleep(true); // Enable WiFi modem sleep for lower power
        esp_wifi_set_ps(WIFI_PS_MIN_MODEM);   // Minimum modem sleep (balanced)
        wifi_ps_type_t ps_mode;
        esp_wifi_get_ps(&ps_mode);
        Serial.printf("[POWER] Set WIFI_PS_MIN_MODEM: %s\n", ps_mode == WIFI_PS_MIN_MODEM ? "ENABLED" : "NOT enabled");

        esp_wifi_set_ps(WIFI_PS_MAX_MODEM);   // Maximum modem sleep (most power saved)
        esp_wifi_get_ps(&ps_mode);
        Serial.printf("[POWER] Set WIFI_PS_MAX_MODEM: %s\n", ps_mode == WIFI_PS_MAX_MODEM ? "ENABLED" : "NOT enabled");

        esp_wifi_set_ps(WIFI_PS_NONE);        // No power save (default high performance)
        esp_wifi_get_ps(&ps_mode);
        Serial.printf("[POWER] Set WIFI_PS_NONE: %s\n", ps_mode == WIFI_PS_NONE ? "ENABLED" : "NOT enabled");

        esp_wifi_set_ps(WIFI_PS_MIN_MODEM); // Explicitly set modem sleep (Arduino ESP32)
        esp_wifi_get_ps(&ps_mode);
        Serial.printf("[POWER] Set WIFI_PS_MIN_MODEM (final): %s\n", ps_mode == WIFI_PS_MIN_MODEM ? "ENABLED" : "NOT enabled");
        if (WiFi.getSleep()) {
            Serial.println("[POWER] WiFi modem sleep is ENABLED.");
        } else {
            Serial.println("[POWER] WiFi modem sleep is NOT enabled!");
        }
        if (!rtc_synced) {
            simple_rtc_sync(); // Sync timezone and RTC on first connect
            rtc_synced = true;
        }
        String ip_address = WiFi.localIP().toString();
        String status_text = "Connected! IP: " + ip_address;
        lv_obj_clear_flag(ui_StatusLabel, LV_OBJ_FLAG_HIDDEN);
        lv_obj_move_foreground(ui_StatusLabel);
        lv_label_set_text(ui_StatusLabel, status_text.c_str());
        lv_obj_set_style_text_color(ui_StatusLabel, lv_color_hex(0x00AA00), LV_PART_MAIN | LV_STATE_DEFAULT);
        wifi_connecting = false;
        // Save credentials to NVS if this was a user-initiated connection
        if (strlen(g_last_user_ssid) > 0 && strlen(g_last_user_password) > 0) {
            save_wifi_credential(g_last_user_ssid, g_last_user_password);
            Serial.printf("[WIFI] Saved user credentials to NVS: SSID='%s'\n", g_last_user_ssid);
        }
        // Reset user WiFi retry count
        g_user_wifi_retry_count = 0;
        // return;
    } else {
        // Reset rtc_synced if WiFi disconnects
        static bool rtc_synced = false;
        rtc_synced = false;
    }

    // If failed or timed out, handle user WiFi retry logic
    if ((status == WL_CONNECT_FAILED || status == WL_NO_SSID_AVAIL || (millis() - wifi_start_time) > wifi_timeout) && strlen(g_last_user_ssid) > 0) {
        if (g_user_wifi_retry_count < 3) {
            g_user_wifi_retry_count++;
            Serial.printf("[WIFI] User WiFi retry %d/3\n", g_user_wifi_retry_count);
            g_user_wifi_pending_disconnect = true;
            g_user_wifi_pending_begin = false;
            wifi_connecting = true;
            wifi_start_time = millis();
            // return;
        } else {
            // After 3 tries, clear user WiFi and move to next stored
            Serial.println("[WIFI] User WiFi failed 3 times, moving to next stored WiFi.");
            g_user_wifi_retry_count = 0;
            g_last_user_ssid[0] = '\0';
            g_last_user_password[0] = '\0';
            g_user_wifi_pending_begin = false;
            g_user_wifi_pending_disconnect = false;
            if (process_try_connect_all_stored_wifi()) {
                Serial.println("[WIFI] Connected to stored WiFi.");
                wifi_connecting = false;
            } else if (!wifi_cred_state.in_progress) {
                // No more credentials or all failed
                lv_label_set_text(ui_StatusLabel, "WiFi not connected");
                lv_obj_set_style_text_color(ui_StatusLabel, lv_color_hex(0xFF0000), LV_PART_MAIN | LV_STATE_DEFAULT);
                wifi_connecting = false;
            }
            // return;
        }
    }
    // If not user WiFi, fallback to original logic for stored WiFi
    // Only try next stored credential if WiFi is not currently connecting
    if ((status == WL_CONNECT_FAILED || status == WL_NO_SSID_AVAIL || (millis() - wifi_start_time) > wifi_timeout)) {
        // Only proceed if WiFi is truly disconnected or failed, not idle (idle means ready, not connecting)
        wl_status_t cur_status = WiFi.status();
        if (cur_status != WL_DISCONNECTED && cur_status != WL_CONNECT_FAILED && cur_status != WL_NO_SSID_AVAIL) {
            // Still connecting or connected, do not try next stored credential yet
            Serial.println("[WIFI] Still connecting or connected, will not try next stored credential yet.");
            return;
        }
        // Now safe to try next stored credential
        if (cur_status == WL_DISCONNECTED || cur_status == WL_CONNECT_FAILED || cur_status == WL_NO_SSID_AVAIL) {
            if (process_try_connect_all_stored_wifi()) {
                Serial.println("[WIFI] Connected to stored WiFi.");
                wifi_connecting = false;
            } else if (!wifi_cred_state.in_progress) {
                // No more credentials or all failed
                lv_label_set_text(ui_StatusLabel, "WiFi not connected");
                lv_obj_set_style_text_color(ui_StatusLabel, lv_color_hex(0xFF0000), LV_PART_MAIN | LV_STATE_DEFAULT);
                wifi_connecting = false;
            }
        }
    }
}