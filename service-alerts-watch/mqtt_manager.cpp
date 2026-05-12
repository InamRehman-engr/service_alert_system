#include "mqtt_manager.h"
#include <Arduino.h>
#include <lvgl.h>
#include "ui.h"
#include <ArduinoJson.h>

// Externs for symbols defined in .ino or elsewhere
extern void beep(float freq, float duration_sec, float vol);
extern void vibrate_haptic(uint16_t ms);
extern bool light_sleep_done;
extern lv_timer_t * washroom_timer;
extern int washroom_seconds_left;
extern lv_obj_t * washroom_ui_timer;
extern lv_obj_t * washroom_ui_label4;



extern const char* mqtt_user;
extern const char* mqtt_pass;
extern const char* mqtt_topic;

void mqtt_reconnect() {
    // Loop until we're reconnected
    while (!mqttClient.connected()) {
        Serial.print("[MQTT] Attempting connection...");
        // Attempt to connect
        String clientId = "TWatchS3-";
        clientId += String(random(0xffff), HEX);
        // Use username and password
        if (mqttClient.connect(clientId.c_str(), mqtt_user, mqtt_pass)) {
            Serial.println("connected");
            mqttClient.subscribe(mqtt_topic);
            Serial.printf("[MQTT] Subscribed to %s\n", mqtt_topic);
            mqttClient.subscribe("alert/ads");
            Serial.println("[MQTT] Subscribed to alert/ads");
            mqttClient.subscribe("alert/kitchen");
            Serial.println("[MQTT] Subscribed to alert/kitchen");
            mqttClient.subscribe("alert/watchok");
            Serial.println("[MQTT] Subscribed to alert/watchok");
        } else {
            Serial.print("failed, rc=");
            Serial.print(mqttClient.state());
            Serial.println(" try again in 5 seconds");
            delay(5000);
        }
    }
}


// C-callable wrapper for MQTT publish for OK button (must be in C++ file)
extern "C" void publish_okbtn_state(int state) {
    if (state == 1) {
        beep(1000, 0.1, 0.6);
        mqttClient.publish("alert/watchok", "1");
        okbtn_state = 1;
        // If on Screen2, allow swipe and lock screen after OK pressed
        extern lv_obj_t * ui_Screen2;
        if (lv_scr_act() == ui_Screen2) {
            block_swipe_and_lockscreen = false;
        }
    } 
}




// Timer callback for washroom countdown
static void washroom_timer_cb(lv_timer_t * timer) {
    extern lv_obj_t * ui_Screen2;
    // Always count down, but only update UI if on Screen2 and UI objects are valid
    if (washroom_seconds_left > 0) {
        washroom_seconds_left--;
        if (lv_scr_act() == ui_Screen2 && washroom_ui_timer && washroom_ui_label4) {
            char tbuf[32];
            int min = washroom_seconds_left / 60;
            int sec = washroom_seconds_left % 60;
            snprintf(tbuf, sizeof(tbuf), "%02d:%02d remaining", min, sec);
            lv_label_set_text(washroom_ui_timer, tbuf);
        }
    } else {
        // Only stop the timer and publish if needed, do not reset labels here
        if (washroom_timer) {
            lv_timer_del(washroom_timer);
            washroom_timer = NULL;
        }
        // If timer finished and user did not press OK (okbtn_state==0), publish last received array to alert/kitchen
        if (okbtn_state == 0) {
            char arr_msg[32];
            snprintf(arr_msg, sizeof(arr_msg), "[%d,%d,%d,%d,%d,%d]", last_alert_button_arr[0], last_alert_button_arr[1], last_alert_button_arr[2], last_alert_button_arr[3], last_alert_button_arr[4], last_alert_button_arr[5]);
            // Append timestamp (seconds since epoch)
            char msg_with_time[64];
            time_t now = time(NULL);
            snprintf(msg_with_time, sizeof(msg_with_time), "%s,%ld", arr_msg, (long)now);
            // Publish with retain flag set to false and QoS 2
            mqttClient.publish("alert/kitchen",(const uint8_t*)msg_with_time, strlen(msg_with_time),true);
        }
        // Do not reset labels here; only do it on alert/ads or OK
    }
}


void mqtt_callback(char* topic, byte* payload, unsigned int length) {
    // Handle alert/ads topic
    if (String(topic) == "alert/ads") {
        alert_ads_received = true;
        light_sleep_done = false; // Reset sleep flag after alert
        // Always perform the same reset logic as OK button
        extern lv_obj_t * ui_Screen2;
        if (washroom_timer) {
            lv_timer_del(washroom_timer);
            washroom_timer = NULL;
        }
        washroom_seconds_left = 0;
        okbtn_state = 0;
        if (lv_scr_act() == ui_Screen2 && washroom_ui_timer && washroom_ui_label4) {
            lv_label_set_text(washroom_ui_timer, "waiting for alert...");
            lv_label_set_text(washroom_ui_label4, "Time to Reach");
            block_swipe_and_lockscreen = false;
        } else {
            block_swipe_and_lockscreen = false;
        }
        return;
    }
    // Convert payload to string
    String msg;
    for (unsigned int i = 0; i < length; i++) {
        msg += (char)payload[i];
    }
    Serial.printf("[MQTT] Message arrived [%s]: %s\n", topic, msg.c_str());

    // Handle alert/button topic with JSON and timestamp check
    if (String(topic) == "alert/button") {
        light_sleep_done = false; // Reset sleep flag after alert
        int arr[6] = {0};
        int washroom = -1;
        bool valid = false;
        long msg_ts = 0;
        // Parse as JSON using ArduinoJson
        StaticJsonDocument<256> doc;
        DeserializationError err = deserializeJson(doc, msg);
        if (!err) {
            if (doc.containsKey("ts")) {
                msg_ts = doc["ts"].as<long>();
            }
            if (doc.containsKey("button") && doc["button"].is<JsonArray>()) {
                JsonArray btn_arr = doc["button"].as<JsonArray>();
                if (btn_arr.size() == 6) {
                    for (int i = 0; i < 6; i++) {
                        arr[i] = btn_arr[i].as<int>();
                        if (arr[i] == 1) {
                            washroom = i + 1;
                            valid = true;
                        }
                    }
                }
            }
        } else {
            // Fallback: old format, parse as before
            int idx = 0;
            for (unsigned int i = 0; i < msg.length() && idx < 6; i++) {
                if (msg[i] == '1' || msg[i] == '0') {
                    arr[idx] = msg[i] - '0';
                    if (arr[idx] == 1) {
                        washroom = idx + 1;
                        valid = true;
                    }
                    idx++;
                }
            }
        }
        // Timestamp check: ignore if too old (older than 50 seconds)
        long now_sec = time(NULL);
        if (msg_ts > 0 && (now_sec - msg_ts) > 70) {
            Serial.printf("[ALERT] Ignoring alert/button: message too old (now=%ld, ts=%ld)\n", now_sec, msg_ts);
            return;
        }
        for (int i = 0; i < 6; i++) last_alert_button_arr[i] = arr[i];
        // Always move to Screen2, even if swipe/lockscreen is blocked
        extern lv_obj_t * ui_Screen2;
        if (ui_Screen2) {
            lv_scr_load(ui_Screen2);
        }
        // If alert_ads_received is false, block swipe/lockscreen
        if (!alert_ads_received) {
            block_swipe_and_lockscreen = true;
        }
        // Show Washroom X and start/continue 2 min timer if valid
        if (valid) {
            Serial.println("[MQTT] Triggering haptic motor!");
            extern lv_obj_t * ui_Label4;
            extern lv_obj_t * ui_timer;
            char label[32];
            snprintf(label, sizeof(label), "Washroom %d", washroom);
            lv_label_set_text(ui_Label4, label);
            lv_obj_set_style_text_font(ui_timer, &lv_font_montserrat_18, 0);
            washroom_ui_timer = ui_timer;
            washroom_ui_label4 = ui_Label4;
            washroom_seconds_left = 120;
            if (!washroom_timer) {
                washroom_timer = lv_timer_create(washroom_timer_cb, 1000, NULL);
            }
            vibrate_haptic(4000); // Vibrate for 4s
        }
        // If all zero, do nothing
        return;
    }
}