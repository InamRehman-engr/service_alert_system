#ifndef MODEM_HPP
#define MODEM_HPP
// Single interface to a modem per application no multi modem for now
#include "driver/gpio.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_modem_api.h"
#include "esp_modem_config.h"
#include "esp_netif.h"
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "string.h"
#include "vfs_resource/vfs_create.hpp"
#include <stdint.h>
#include <time.h>

typedef struct modem_t {
  struct modem_status_t {
    int rssi, ber;
    char imei[14];          // You can use a character array for imei instead of
                            // std::string
    char imsi[14];          // Character array for imsi
    char operator_name[30]; // Character array for operator_name
    enum modem_data_mode_state_t {
      MODEM_DATA_MODE_STATE_HAS_IP,
      MODEM_DATA_MODE_STATE_CONNECTING,
      MODEM_DATA_MODE_STATE_DISCONNECTED,
      MODEM_UNAVAILABLE
    } data_state;
    bool available;
  } modem_status;
  bool run_thread;
  esp_modem_dce_mode_t current_mode;
  esp_modem_dce_mode_t desired_mode;
  esp_modem_dte_config_t dte_config; // User needs to set these

  esp_netif_t *esp_netif;
  esp_modem_dce_t *dce;
  TaskHandle_t task_handle;
  esp_err_t (*sendTextMessage)(struct modem_t *modem, const char *number,
                               size_t number_length, const char *message,
                               size_t message_length);
  bool dce_initialized;
  char *module_name;
};

void init_modem(void);
void setModemDataState(esp_modem_dce_mode_t mode);
void deinit_modem(void);
#ifdef CONFIG_ENABLE_GSM
void init_gnss(void (*on_data_callback)(esp_modem_gps_t *));
void deinit_gnss(void);
#endif
#endif

/// TODO: Add GNSS Handler for available modems