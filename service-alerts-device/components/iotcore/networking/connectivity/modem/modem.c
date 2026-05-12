
#include "modem.h"

#if defined(CONFIG_FLOW_CONTROL_NONE)
#define FLOW_CONTROL ESP_MODEM_FLOW_CONTROL_NONE
#elif defined(CONFIG_FLOW_CONTROL_SW)
#define FLOW_CONTROL ESP_MODEM_FLOW_CONTROL_SW
#elif defined(CONFIG_FLOW_CONTROL_HW)
#define FLOW_CONTROL ESP_MODEM_FLOW_CONTROL_HW
#endif
///  TODO: Optimize modem initialization time
static const char *TAG = "Modem Handler";
static struct modem_t modem;
static esp_modem_gps_t modem_gps_data;
static TaskHandle_t gps_task_handle;
typedef struct {
  char *device_name;
  esp_modem_dce_device_t device_type;
} device_lookup_table;

static const device_lookup_table device_lookup_table_list[] = {
    {"BG96", ESP_MODEM_DCE_BG96},

    {"A7672", ESP_MODEM_DCE_A76XX},     {"A7670", ESP_MODEM_DCE_A76XX},

    {"SIM800", ESP_MODEM_DCE_SIM800},   {"SIM868", ESP_MODEM_DCE_SIM800},

    {"SIM7000", ESP_MODEM_DCE_SIM7000}, {"SIM900", ESP_MODEM_DCE_SIM7000},
    {"SIM800F", ESP_MODEM_DCE_SIM7000},

    {"SIM7500", ESP_MODEM_DCE_SIM7600}, {"SIM7600", ESP_MODEM_DCE_SIM7600},

    {"SIM7070", ESP_MODEM_DCE_SIM7070}, {"SIM7080", ESP_MODEM_DCE_SIM7070},
    {"SIM7090", ESP_MODEM_DCE_SIM7070},
};
/*
    Returns true if state of modem at the end of calling this is on and false if
   undefined
*/
esp_err_t power_cycle_modem(void) {
  // Powerkey cycle
#if (CONFIG_MODEM_PWRKEY != -1) // For SIM868
  gpio_num_t switch_pin = (gpio_num_t)CONFIG_MODEM_PWRKEY;
#elif (CONFIG_MODEM_POWER_SWITCH_KEY !=                                        \
       -1) // You need to make sure that doing this in hardware will actually
           // reset or boot up the modem
  gpio_num_t switch_pin = (gpio_num_t)CONFIG_MODEM_POWER_SWITCH_KEY;
#else
#error "Need atleast one way to control power to modem"
  gpio_num_t switch_pin = GPIO_NUM_NC;
#endif
  if (switch_pin != GPIO_NUM_NC) {
    gpio_set_direction(switch_pin, GPIO_MODE_OUTPUT);
    gpio_set_level(switch_pin, 0);
    vTaskDelay(pdMS_TO_TICKS(1000));
    gpio_set_level(switch_pin, 1);
    vTaskDelay(pdMS_TO_TICKS(4000));
    return ESP_OK;
  }
  return ESP_FAIL;
}

bool change_modem_mode(enum esp_modem_dce_mode mode) {
  for (int x = 0; x < 10; x++) {
    if (esp_modem_set_mode(modem.dce, mode) == ESP_OK) {
      ESP_LOGI(TAG,
               "Modem has correctly entered multiplexed command/data mode");
      modem.current_mode = mode;
      return true;
    } else {
      ESP_LOGE(TAG,
               "Failed to configure multiplexed command/data mode... retrying");
    }
    vTaskDelay(pdMS_TO_TICKS(1000));
  }
  return false;
}
esp_err_t esp_modem_set_dce(char *current_module_name) {
  if (current_module_name == NULL) {
    ESP_LOGW(TAG, "No module name");
    return ESP_FAIL;
  }
  ESP_LOGI(TAG, "Module: %s", current_module_name);
  modem.dce_initialized = false;
  char *parse_position = strchr(current_module_name, '_');
  if (parse_position != NULL) {
    strncpy(current_module_name, parse_position + 1,
            strlen(parse_position) + 1);
  }
  for (int x = 0;
       x < sizeof(device_lookup_table_list) / sizeof(device_lookup_table);
       x++) {
    if (strncmp(current_module_name, device_lookup_table_list[x].device_name,
                strlen(device_lookup_table_list[x].device_name)) == 0) {
      ESP_LOGI(TAG, "Initializing esp_modem for the %s module...",
               current_module_name);
      esp_modem_destroy(modem.dce);
      esp_modem_dce_config_t dce_config =
          ESP_MODEM_DCE_DEFAULT_CONFIG(CONFIG_MODEM_PPP_APN);
      modem.dce =
          esp_modem_new_dev(device_lookup_table_list[x].device_type,
                            &modem.dte_config, &dce_config, modem.esp_netif);
      assert(modem.dce);
      strcpy(modem.module_name, current_module_name);
      return ESP_OK;
    }
  }
  ESP_LOGW(TAG, "Unknown module");
  return ESP_FAIL;
}
void gps_task(void (*on_data_callback)(esp_modem_gps_t *)) {
  esp_err_t err = ESP_FAIL;
  int gnss_mode = 0;
  while (1) {
    while (modem.dce_initialized == false) {
      vTaskDelay(pdMS_TO_TICKS(1000));
    }
    err = esp_modem_get_gnss_power_mode(modem.dce, &gnss_mode);
    if (err == ESP_OK && gnss_mode == 1) {
      ESP_LOGI(TAG, "GNSS Already Enabled");
    } else if (gnss_mode == 0) {
      if (esp_modem_set_gnss_power_mode(modem.dce, 1) != ESP_OK) {
        ESP_LOGE(TAG, "GNSS Power Enable Failed");
        vTaskDelay(pdMS_TO_TICKS(1000));
        continue;
      }
      ESP_LOGI(TAG, "GNSS Enable Successful");
    } else {
      ESP_LOGE(TAG, "esp_modem_get_gnss_power_mode failed with reason %s",
               esp_err_to_name(err));
    }
    while (modem.dce_initialized == true) {
      err = esp_modem_get_gps_location(modem.dce, &modem_gps_data);
      if (err != ESP_OK) {
        ESP_LOGE(TAG, "GPS Get/Parse failed");
        vTaskDelay(pdMS_TO_TICKS(10000));
        continue;
      }
      ESP_LOGD(TAG, "UTC Time (ms): %llu", modem_gps_data.gps_time_ms);
      ESP_LOGD(TAG, "Latitude: %f degrees", modem_gps_data.latitude);
      ESP_LOGD(TAG, "Longitude: %f degrees", modem_gps_data.longitude);
      ESP_LOGD(TAG, "Altitude: %f meters", modem_gps_data.altitude);
      ESP_LOGD(TAG, "Run Type: %u", modem_gps_data.run);
      ESP_LOGD(TAG, "Fix Type: %u", modem_gps_data.fix);
      ESP_LOGD(TAG, "Fix Mode: %u", modem_gps_data.fix_mode);
      ESP_LOGD(TAG, "Satellites in Use: %u", modem_gps_data.sats_in_use);
      ESP_LOGD(TAG, "Satellites in Use: %u", modem_gps_data.sats_in_use);
      ESP_LOGD(TAG, "Horizontal DOP: %f", modem_gps_data.dop_h);
      ESP_LOGD(TAG, "Position DOP: %f", modem_gps_data.dop_p);
      ESP_LOGD(TAG, "Vertical DOP: %f", modem_gps_data.dop_v);
      ESP_LOGD(TAG, "Speed: %f", modem_gps_data.speed);
      ESP_LOGD(TAG, "Course over Ground: %f degrees", modem_gps_data.cog);
      ESP_LOGD(TAG, "Atmospheric Pressure (Sea Level): %f hPa",
               modem_gps_data.hpa);
      ESP_LOGD(TAG, "Atmospheric Pressure (Current Location): %f hPa",
               modem_gps_data.vpa);
      ESP_LOGD(TAG, "Carrier to Noise Density Ratio (C/No): %u dBHz",
               modem_gps_data.carrier_noise);
      ESP_LOGD(TAG, "Direction: %u", modem_gps_data.direction);
      on_data_callback(&modem_gps_data);
      vTaskDelay(pdMS_TO_TICKS(10000));
    }
  }
}
esp_err_t modem_sync_handler() {
  esp_err_t err = ESP_FAIL;
  for (int x = 0; x < 5; x++) {
    err = esp_modem_sync(modem.dce);
    if (err == ESP_OK) {
      return err;
    }
    vTaskDelay(pdMS_TO_TICKS(1000));
  }
  return err;
}

void modem_task(void *pvParameters) {
  /// TODO: Handle reconnection of usb modem.
  char current_module_name[30] = "\0";
  modem.module_name = malloc(sizeof(current_module_name));
  esp_err_t connected_status = esp_modem_sync(modem.dce);
  while (1) {
  start:
    modem.dce_initialized = false;
    esp_modem_destroy(modem.dce);
    modem.current_mode = ESP_MODEM_MODE_DATA;
    esp_modem_dce_config_t dce_config =
        ESP_MODEM_DCE_DEFAULT_CONFIG(CONFIG_MODEM_PPP_APN);
    ESP_LOGI(TAG, "Initializing esp_modem for a generic module...");
    modem.dce = esp_modem_new(&modem.dte_config, &dce_config, modem.esp_netif);
    assert(modem.dce);
    power_cycle_modem();
    uint64_t delay = time(NULL);
    while (connected_status != ESP_OK) {
      ESP_LOGW(TAG, "Modem not connected");
      if (time(NULL) - delay > 60) // 60 seconds
      {
        ESP_LOGW(TAG, "Power Cycling Modem");
        power_cycle_modem();
        delay = time(NULL);
      }
      connected_status = esp_modem_sync(modem.dce);
      vTaskDelay(pdMS_TO_TICKS(1000));
    }
    ESP_LOGI(TAG, "Modem Connected.... Initializing DCE");
    memset(current_module_name, 0, sizeof(current_module_name));
    memset(modem.module_name, 0, sizeof(current_module_name));
    while (modem.dce_initialized == false) {
      connected_status = modem_sync_handler();
      if (connected_status != ESP_OK) {
        ESP_LOGW(TAG, "Modem disconnected");
        goto start;
      }
      if (esp_modem_get_module_name(modem.dce, current_module_name) == ESP_OK) {
        if (esp_modem_set_dce(current_module_name) != ESP_OK) {
          vTaskDelay(pdMS_TO_TICKS(2000));
          continue;
        }
        modem.dce_initialized = true;
      }
      vTaskDelay(pdMS_TO_TICKS(1000));
    }
    memset(modem.modem_status.imei, 0, sizeof(modem.modem_status.imei));
    while (modem.modem_status.imei[0] == '\0') {
      connected_status = modem_sync_handler();
      if (connected_status != ESP_OK) {
        ESP_LOGW(TAG, "Modem disconnected");
        goto start;
      }
      esp_modem_get_imei(modem.dce, modem.modem_status.imei);
      ESP_LOGI(TAG, "IMEI: %s", modem.modem_status.imei);
      vTaskDelay(pdMS_TO_TICKS(1000));
    }
    memset(modem.modem_status.imsi, 0, sizeof(modem.modem_status.imsi));
    while (modem.modem_status.imsi[0] == '\0') {
      connected_status = modem_sync_handler();
      if (connected_status != ESP_OK) {
        ESP_LOGW(TAG, "Modem disconnected");
        goto start;
      }
      esp_modem_get_imsi(modem.dce, modem.modem_status.imsi);
      ESP_LOGI(TAG, "IMSI: %s", modem.modem_status.imsi);
      vTaskDelay(pdMS_TO_TICKS(1000));
    }
    memset(modem.modem_status.operator_name, 0,
           sizeof(modem.modem_status.operator_name));
    while (modem.modem_status.operator_name[0] == '\0') {
      connected_status = modem_sync_handler();
      if (connected_status != ESP_OK) {
        ESP_LOGW(TAG, "Modem disconnected");
        goto start;
      }
      int access_technology = 0;
      esp_modem_get_operator_name(modem.dce, modem.modem_status.operator_name,
                                  &access_technology);
      ESP_LOGI(TAG, "Operator: %s", modem.modem_status.operator_name);
      vTaskDelay(pdMS_TO_TICKS(1000));
    }
    modem.modem_status.available = true;
    while (modem.run_thread) {
      connected_status = modem_sync_handler();
      if (connected_status != ESP_OK) {
        ESP_LOGW(TAG, "Modem disconnected");
        modem.modem_status.available = false;
        goto start;
      }
      // Only thing i can think of doing with this for now is to check for
      // rssi here
      esp_modem_get_signal_quality(modem.dce, &modem.modem_status.rssi,
                                   &modem.modem_status.ber);
      ESP_LOGI(TAG, "RSSI: %d, BER: %d", modem.modem_status.rssi,
               modem.modem_status.ber);

      // Need to check error here and restart the module hardware
      if (modem.current_mode != modem.desired_mode) {
        if (modem.modem_status.rssi != 0) {
          if (change_modem_mode(modem.desired_mode) ==
              false) {                            // Modem did not change modes
            modem.modem_status.available = false; // Not in data mode
          }
        }
      }
      vTaskDelay(pdMS_TO_TICKS(5000));
    }
  }
}

/*
  Text Message sending

*/
esp_err_t sendTextMessage(const char *number, const char *message) {
  esp_modem_dce_t *dce = modem.dce;
  esp_err_t err = esp_modem_send_sms(dce, number, message);
  return err;
}

/*
    Initialize event loop and netif handler before doing this.
    This will initialize modem by power cycle and will block for almost 3-5
   seconds

*/
void init_modem(void) {
  memset(&modem, 0, sizeof(struct modem_t));
  modem.dte_config = (esp_modem_dte_config_t)ESP_MODEM_DTE_DEFAULT_CONFIG();
  modem.dte_config.uart_config.rx_io_num = CONFIG_MODEM_UART_RX_PIN;
  modem.dte_config.uart_config.tx_io_num = CONFIG_MODEM_UART_TX_PIN;
#if CONFIG_FLOW_CONTROL == CONFIG_FLOW_CONTROL_HW
  modem.dte_config.uart_config.rts_io_num = CONFIG_MODEM_UART_RTS_PIN;
  modem.dte_config.uart_config.cts_io_num = CONFIG_MODEM_UART_CTS_PIN;
#else
  modem.dte_config.uart_config.rts_io_num = -1;
  modem.dte_config.uart_config.cts_io_num = -1;
#endif
  if (modem.dte_config.uart_config.flow_control == ESP_MODEM_FLOW_CONTROL_HW) {
    esp_err_t err = esp_modem_set_flow_control(modem.dce, 2,
                                               2); // 2/2 means HW Flow Control.
    if (err != ESP_OK) {
      ESP_LOGE(TAG, "Failed to set the set_flow_control mode");
      return;
    }
    ESP_LOGI(TAG, "HW set_flow_control OK");
  }
  modem.run_thread = true;
  modem.current_mode = -1;
  modem.modem_status.rssi = 0;
  esp_netif_config_t netif_ppp_config = ESP_NETIF_DEFAULT_PPP();
  modem.esp_netif = esp_netif_new(&netif_ppp_config);
  assert(modem.esp_netif);
#if defined(CONFIG_MODEM_SERIAL_CONFIG_USB)
///  TODO: Refactor USB Config
#if CONFIG_MODEM_DEVICE_BG96 == 1
  ESP_LOGI(TAG, "Initializing esp_modem for the BG96 module...");
  struct esp_modem_usb_term_config usb_config = ESP_MODEM_BG96_USB_CONFIG();
  esp_modem_dce_device_t usb_dev_type = ESP_MODEM_DCE_BG96;
#elif CONFIG_MODEM_DEVICE_SIM7600 == 1
  ESP_LOGI(TAG, "Initializing esp_modem for the SIM7600 module...");
  struct esp_modem_usb_term_config usb_config = ESP_MODEM_SIM7600_USB_CONFIG();
  esp_modem_dce_device_t usb_dev_type = ESP_MODEM_DCE_SIM7600;
#elif CONFIG_MODEM_DEVICE_A7670 == 1
  ESP_LOGI(TAG, "Initializing esp_modem for the A7670 module...");
  struct esp_modem_usb_term_config usb_config = ESP_MODEM_A7670_USB_CONFIG();
  esp_modem_dce_device_t usb_dev_type = ESP_MODEM_DCE_SIM7600;
#else
#error USB modem not selected
#endif
  esp_modem_dce_config_t dce_config =
      ESP_MODEM_DCE_DEFAULT_CONFIG(CONFIG_MODEM_PPP_APN);
  const esp_modem_dte_config_t dte_usb_config =
      ESP_MODEM_DTE_DEFAULT_USB_CONFIG(usb_config);
  ESP_LOGI(TAG, "Waiting for USB device connection...");
  esp_modem_dce_t *dce = esp_modem_new_dev_usb(
      usb_dev_type, &dte_usb_config, &modem.dce_config, modem.esp_netif);
  assert(dce);
  esp_modem_set_error_cb(dce, usb_terminal_error_handler);
  ESP_LOGI(TAG, "Modem connected, waiting 10 seconds for boot...");
  vTaskDelay(pdMS_TO_TICKS(10000)); // Give DTE some time to boot
#endif
  modem.current_mode = ESP_MODEM_MODE_COMMAND;
  modem.desired_mode = ESP_MODEM_MODE_CMUX;
  xTaskCreate(modem_task, "modem_task", 1024 * 5, NULL, 5, &modem.task_handle);
}
void setModemDataState(esp_modem_dce_mode_t mode) { modem.desired_mode = mode; }
void deinit_modem(void) {
  free(modem.module_name);
  vTaskDelete(modem.task_handle);
  esp_modem_destroy(modem.dce);
  esp_netif_destroy(modem.esp_netif);
}
#ifdef CONFIG_ENABLE_GSM
void init_gnss(void (*on_data_callback)(esp_modem_gps_t *)) {
  if (on_data_callback == NULL) {
    ESP_LOGE(TAG,
             "Null Function Pointer to on_data_callback for gnss.... aborting");
    return;
  }
  xTaskCreate(gps_task, "gps_task", 1024 * 5, on_data_callback, 5,
              &gps_task_handle);
}
void deinit_gnss(void) { vTaskDelete(gps_task_handle); }

#endif