
#ifndef WIFI_CREDENTIALS_H
#define WIFI_CREDENTIALS_H

#include <Arduino.h>

#define MAX_WIFI_CREDENTIALS 10
#define WIFI_NAMESPACE "wifi_creds"

// WiFi related variables (declare only, define in .ino)
extern char wifi_ssid[64];
extern char wifi_password[64];
extern bool wifi_connecting;
extern unsigned long wifi_start_time;
extern const unsigned long wifi_timeout;
extern int g_user_wifi_retry_count;
extern char g_last_user_ssid[64];
extern char g_last_user_password[64];
extern bool g_user_wifi_pending_disconnect;


struct WifiCredential {
    String ssid;
    String password;
};

struct WifiCredentialState {
    WifiCredential creds[MAX_WIFI_CREDENTIALS];
    int count;
    int current;
    unsigned long attempt_start;
    bool in_progress;
};

extern WifiCredentialState wifi_cred_state;

#ifdef __cplusplus
extern "C" {
#endif

void save_wifi_credential(const char* ssid, const char* password);
int load_wifi_credentials(WifiCredential creds[MAX_WIFI_CREDENTIALS]);
void clear_wifi_credentials();
void start_try_connect_all_stored_wifi();
bool process_try_connect_all_stored_wifi();
void check_wifi_status();
void connect_to_wifi(const char* ssid, const char* password);

#ifdef __cplusplus
}
#endif

#endif // WIFI_CREDENTIALS_H