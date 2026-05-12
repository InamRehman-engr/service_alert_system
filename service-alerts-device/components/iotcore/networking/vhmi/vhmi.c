#include "vhmi.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "freertos/queue.h"
#include "freertos/task.h"
#include "sdkconfig.h"

#include "sdkconfig.h"

#define TAG "VHMI"

#define DebugPrints
#define PRINT
#define Err_check_prints

#if defined(DebugPrints) && defined(PRINT)
#define print_d printf
#define printd_HEXDUMP ESP_LOG_BUFFER_HEXDUMP
#else
#define printd(...)
#define printd_HEXDUMP(...)
#endif

#if defined(Err_check_prints) && defined(PRINT)
#define print_err ESP_LOGE
#define printd_HEXDUMP ESP_LOG_BUFFER_HEXDUMP
#else
#define printd(...)
#define printd_HEXDUMP(...)
#endif

#if defined(Err_check_prints) && defined(PRINT)
#define print_logI ESP_LOGI
#define printd_HEXDUMP ESP_LOG_BUFFER_HEXDUMP
#else
#define printd(...)
#define printd_HEXDUMP(...)
#endif
uint8_t vhmi_call_type =
    -1; ///     0 -> from Mqtt, 1-> from socket, 2-> from BLE
extern uint8_t is_ble_connected;
extern uint8_t is_socket_connected;
void data_recived(int32_t len, uint8_t *data);

#ifdef CONFIG_FINGER_PRINT
extern EventGroupHandle_t fingerprint_state_event_group;

extern const int User_enrollement_STATE_BIT;
extern const int Finger_search_STATE_BIT;
extern const int get_no_of_users_STATE_BIT;
extern const int change_password_STATE_BIT;
extern const int username_STATE_BIT;
extern const int FingerPrint_pasword_STATE_BIT;
extern const int user_id_STATE_BIT;
extern const int new_password_STATE_BIT;
extern const int delete_all_users_STATE_BIT;
extern const int delete_single_user_STATE_BIT;
extern const int Scanner_Online_Status_STATE_BIT;

extern uint32_t password_from_vhmi;
extern uint32_t New_password_from_vhmi;
extern char username_from_vhmi[50];
extern uint8_t username_len_from_vhmi;
extern uint16_t userId_from_vhmi;

#endif

#ifdef CONFIG_WATER_TANK_LEVEL
extern EventGroupHandle_t Water_level_tank_state_event_group;

extern const int set_vhmi_settings_task_STATE_BIT;
extern const int get_vhmi_settings_task_STATE_BIT;
#endif
#ifdef CONFIG_MOTOR_SWITCH
extern uint8_t tank_settings_from_vhmi[];
#endif

int mqtt_publish_vhmi_data(uint16_t size, uint8_t *data);
void data_recived_mqtt(int32_t len, uint8_t *data) {
  vhmi_call_type = 0;
  data_recived(len, data);
}

esp_err_t tcpsocket_publish_vhmi_data(uint16_t size, uint8_t *data);
void tcpsocket_configure(int online_sockets, char *ssid, int len);
void data_recived_socket(int32_t len, uint8_t *data) {
  vhmi_call_type = 1;
  data_recived(len, data);
}

esp_err_t senddataonble(uint16_t size, uint8_t *data);
void Configure_ble(int set_ble);
void data_recived_on_ble(int32_t len, uint8_t *data) {
  vhmi_call_type = 2;
  data_recived(len, data);
}

void data_recived(int32_t len, uint8_t *data) {
  ESP_LOGI(TAG, " data_recived_vhmi len %ld ", len);
  ESP_LOG_BUFFER_HEXDUMP(TAG, data, len, ESP_LOG_INFO);

  int remainlen = len;
  vhmi_cmd_t *cmd = (vhmi_cmd_t *)data;
  uint8_t *pointer = data;
  if (data == NULL) {
    ESP_LOGE(TAG, "data pointer is not initialize   ");
    return;
  }
  while (remainlen > 0) {
    cmd = (vhmi_cmd_t *)pointer;
    if (cmd->len < remainlen) {
      ESP_LOGE(TAG, "Packet len(%d) is less than it should be (%d)  ",
               remainlen, cmd->len);
      return;
    }
    print_d("cmd->cmd_type %d\n", cmd->cmd_type);
    switch (cmd->cmd_type) {
    case vhmi_cmd_type_buttonpressed:
#ifdef BUTTON_INCLUDED
      print_d("vhmi_cmd_type_buttonpressed %d [%d]\n",
              cmd->buttonpressed.button, cmd->buttonpressed.pressedfor);
      buttonPressed_ext(cmd->buttonpressed.button,
                        cmd->buttonpressed.pressedfor);
#endif
      break;

    case vhmi_cmd_type_get_BLE_online_status:
      print_d("vhmi_cmd_type_get_BLE_online_status\n");
      Configure_ble(cmd->Ble.Config);
      break;

    case vhmi_cmd_type_set_socket_mode:
      print_d("vhmi_cmd_type_set_socket_mode\n");

      if (cmd->len < 4 && cmd->socket_mode.Config == 0) {
        tcpsocket_configure(cmd->socket_mode.Config, 0, 0); // stop tcp socket
      } else {
        char wifi_ssid[50];
        memcpy(wifi_ssid, cmd->socket_mode.wifi_ssid, cmd->len - 3);
        print_d("\n ssid =  %s \n", wifi_ssid);
        tcpsocket_configure(cmd->socket_mode.Config, wifi_ssid, cmd->len - 3);
      }

      break;

    case vhmi_cmd_type_switch_channel:
      print_d("vhmi_cmd_type_switch_channel\n");
      vhmi_call_type = cmd->comm_channel.swtch;
      break;
    case vhmi_cmd_type_shutdown_communication_channel:
      shuttdown_communication_channel();
      break;

#if defined(CONFIG_WATER_TANK_LEVEL) && defined(CONFIG_MOTOR_SWITCH)
    case vhmi_cmd_type_tank_parameter_settings:
      print_d("vhmi_cmd_type_tank_parameter_settings\n");
      memcpy(tank_settings_from_vhmi, &cmd->tank_parameters_settings,
             cmd->len - 2);
      if (Water_level_tank_state_event_group)
        xEventGroupSetBits(Water_level_tank_state_event_group,
                           set_vhmi_settings_task_STATE_BIT);
      break;
    case vhmi_cmd_type_get_tank_parameter_settings:
      xEventGroupSetBits(Water_level_tank_state_event_group,
                         get_vhmi_settings_task_STATE_BIT);
      break;
#endif

#ifdef CONFIG_FINGER_PRINT
    case vhmi_cmd_type_Add_user:
      print_d("vhmi_cmd_type_Add_user\n");

      //            vTaskSuspend(fingerprint_Search_task_Handle);
      xEventGroupClearBits(fingerprint_state_event_group,
                           delete_all_users_STATE_BIT);
      xEventGroupClearBits(fingerprint_state_event_group,
                           get_no_of_users_STATE_BIT);
      xEventGroupClearBits(fingerprint_state_event_group,
                           Finger_search_STATE_BIT);
      xEventGroupClearBits(fingerprint_state_event_group,
                           delete_single_user_STATE_BIT);
      xEventGroupClearBits(fingerprint_state_event_group,
                           change_password_STATE_BIT);
      xEventGroupClearBits(fingerprint_state_event_group,
                           Scanner_Online_Status_STATE_BIT);

      xEventGroupSetBits(fingerprint_state_event_group,
                         User_enrollement_STATE_BIT);

      break;
    case vhmi_cmd_type_scanner_online_status:
      printf("vhmi_cmd_type_scanner_online_status\n");

      xEventGroupClearBits(fingerprint_state_event_group,
                           delete_all_users_STATE_BIT);
      xEventGroupClearBits(fingerprint_state_event_group,
                           get_no_of_users_STATE_BIT);
      xEventGroupClearBits(fingerprint_state_event_group,
                           Finger_search_STATE_BIT);
      xEventGroupClearBits(fingerprint_state_event_group,
                           delete_single_user_STATE_BIT);
      xEventGroupClearBits(fingerprint_state_event_group,
                           change_password_STATE_BIT);
      xEventGroupClearBits(fingerprint_state_event_group,
                           User_enrollement_STATE_BIT);

      xEventGroupSetBits(fingerprint_state_event_group,
                         Scanner_Online_Status_STATE_BIT);

      // respond_scanner_online_status();
      break;
    case vhmi_cmd_type_Username:
      printf("vhmi_cmd_type_Username\n");

      memcpy(username_from_vhmi, cmd->User.user_name, cmd->len - 2);

      username_len_from_vhmi = cmd->len - 2;

      print_d("\n username from vhmi =  %s \n", username_from_vhmi);

      xEventGroupSetBits(fingerprint_state_event_group, username_STATE_BIT);

      print_d("\nbit set\n");

      break;
    case vhmi_cmd_type_Enter_Password:
      printf("vhmi_cmd_type_Enter_password\n");

      memcpy(&password_from_vhmi, &cmd->Enter_password.password, 4);

      printd_HEXDUMP("Password", &password_from_vhmi, 4, LOG_LOCAL_LEVEL);

      xEventGroupSetBits(fingerprint_state_event_group,
                         FingerPrint_pasword_STATE_BIT);

      print_d("\nbit set\n");

      //    fingerprint_enter_password(&cmd->Enter_password.password);
      break;
    case vhmi_cmd_type_User_added:
      printf("vhmi_cmd_type_User_added\n");
      break;
    case vhmi_cmd_type_UserId:
      printf("vhmi_cmd_type_UserId\n");

      print_d("\nUser Id to delete = %d\n", cmd->UserToDel.user_id);

      userId_from_vhmi = cmd->UserToDel.user_id;

      print_d("\nUser Id to delete = %d\n", userId_from_vhmi);
      xEventGroupSetBits(fingerprint_state_event_group, user_id_STATE_BIT);

      break;

    case vhmi_cmd_type_Delete_Single_user:
      printf("vhmi_cmd_type_Delete_Single_user\n");

      xEventGroupClearBits(fingerprint_state_event_group,
                           User_enrollement_STATE_BIT);
      xEventGroupClearBits(fingerprint_state_event_group,
                           get_no_of_users_STATE_BIT);
      xEventGroupClearBits(fingerprint_state_event_group,
                           delete_all_users_STATE_BIT);
      xEventGroupClearBits(fingerprint_state_event_group,
                           Finger_search_STATE_BIT);
      xEventGroupClearBits(fingerprint_state_event_group,
                           change_password_STATE_BIT);
      xEventGroupClearBits(fingerprint_state_event_group,
                           Scanner_Online_Status_STATE_BIT);

      xEventGroupSetBits(fingerprint_state_event_group,
                         delete_single_user_STATE_BIT);

      break;
    case vhmi_cmd_type_Auth_status:
      printf("vhmi_cmd_type_Auth_status\n");
      break;
    case vhmi_cmd_type_Finger_place:
      printf("vhmi_cmd_type_Finger_place\n");
      break;
    case vhmi_cmd_type_Finger_remove:
      printf("vhmi_cmd_type_Finger_remove\n");
      break;
    case vhmi_cmd_type_scanner_error:
      printf("vhmi_cmd_type_finger_error\n");
      break;
    case vhmi_cmd_type_fingerprint_matched:
      printf("vhmi_cmd_type_fingerprint_added\n");
      break;
    case vhmi_cmd_type_fingerprint_deleted:
      printf("vhmi_cmd_type_fingerprint_deleted\n");
      break;
    case vhmi_cmd_type_delete_all_users:
      printf("vhmi_cmd_type_delete_all_users\n");
      xEventGroupClearBits(fingerprint_state_event_group,
                           User_enrollement_STATE_BIT);
      xEventGroupClearBits(fingerprint_state_event_group,
                           get_no_of_users_STATE_BIT);
      xEventGroupClearBits(fingerprint_state_event_group,
                           Finger_search_STATE_BIT);
      xEventGroupClearBits(fingerprint_state_event_group,
                           delete_single_user_STATE_BIT);
      xEventGroupClearBits(fingerprint_state_event_group,
                           change_password_STATE_BIT);
      xEventGroupClearBits(fingerprint_state_event_group,
                           Scanner_Online_Status_STATE_BIT);

      xEventGroupSetBits(fingerprint_state_event_group,
                         delete_all_users_STATE_BIT);

      break;
    case vhmi_cmd_type_Get_user:
      printf("vhmi_cmd_type_Get_user\n");
      xEventGroupClearBits(fingerprint_state_event_group,
                           User_enrollement_STATE_BIT);
      xEventGroupClearBits(fingerprint_state_event_group,
                           delete_all_users_STATE_BIT);
      xEventGroupClearBits(fingerprint_state_event_group,
                           Finger_search_STATE_BIT);
      xEventGroupClearBits(fingerprint_state_event_group,
                           delete_single_user_STATE_BIT);
      xEventGroupClearBits(fingerprint_state_event_group,
                           change_password_STATE_BIT);
      xEventGroupClearBits(fingerprint_state_event_group,
                           Scanner_Online_Status_STATE_BIT);

      xEventGroupSetBits(fingerprint_state_event_group,
                         get_no_of_users_STATE_BIT);
      break;
    case vhmi_cmd_type_Get_number_of_user:
      printf("vhmi_cmd_type_Get_number_of_user\n");
      break;
    case vhmi_cmd_type_Change_password:
      printf("vhmi_cmd_type_Change_password\n");

      xEventGroupClearBits(fingerprint_state_event_group,
                           User_enrollement_STATE_BIT);
      xEventGroupClearBits(fingerprint_state_event_group,
                           delete_all_users_STATE_BIT);
      xEventGroupClearBits(fingerprint_state_event_group,
                           Finger_search_STATE_BIT);
      xEventGroupClearBits(fingerprint_state_event_group,
                           delete_single_user_STATE_BIT);
      xEventGroupClearBits(fingerprint_state_event_group,
                           get_no_of_users_STATE_BIT);
      xEventGroupClearBits(fingerprint_state_event_group,
                           Scanner_Online_Status_STATE_BIT);

      xEventGroupSetBits(fingerprint_state_event_group,
                         change_password_STATE_BIT);

      break;

    case vhmi_cmd_type_Password_changed:
      printf("vhmi_cmd_type_Password_changed\n");
      break;
    case vhmi_cmd_type_get_out_of_enroll:
      printf("vhmi_cmd_type_get_out_of_enroll\n");
      //    vTaskResume(fingerprint_Search_task_Handle);
      xEventGroupClearBits(fingerprint_state_event_group,
                           User_enrollement_STATE_BIT);
      xEventGroupSetBits(fingerprint_state_event_group,
                         Finger_search_STATE_BIT);
      shuttdown_communication_channel();
      break;

    case vhmi_cmd_type_get_out_of_noOfusersMode:
      printf("vhmi_cmd_type_get_out_of_noOfusersMode\n");

      xEventGroupClearBits(fingerprint_state_event_group,
                           get_no_of_users_STATE_BIT);
      xEventGroupSetBits(fingerprint_state_event_group,
                         Finger_search_STATE_BIT);
      shuttdown_communication_channel();
      break;
    case vhmi_cmd_type_get_out_from_deleteSingleuserMode:
      printf("vhmi_cmd_type_get_out_from_deleteSingleuserMode\n");
      xEventGroupClearBits(fingerprint_state_event_group,
                           delete_single_user_STATE_BIT);
      xEventGroupSetBits(fingerprint_state_event_group,
                         Finger_search_STATE_BIT);
      shuttdown_communication_channel();
      break;
    case vhmi_cmd_type_new_pass:
      printf("vhmi_cmd_type_new_pass\n");

      memcpy(&New_password_from_vhmi, &cmd->New_password.password, 4);

      printd_HEXDUMP("new Password from vhmi ", &New_password_from_vhmi, 4,
                     LOG_LOCAL_LEVEL);

      xEventGroupSetBits(fingerprint_state_event_group, new_password_STATE_BIT);
      break;

    case vhmi_cmd_type_get_out_from_New_Pass_mode:
      xEventGroupClearBits(fingerprint_state_event_group,
                           change_password_STATE_BIT);
      xEventGroupSetBits(fingerprint_state_event_group,
                         Finger_search_STATE_BIT);
      shuttdown_communication_channel();
      break;

    case vhmi_cmd_type_get_out_of_deleteAllusersMode:
      xEventGroupClearBits(fingerprint_state_event_group,
                           delete_all_users_STATE_BIT);
      xEventGroupSetBits(fingerprint_state_event_group,
                         Finger_search_STATE_BIT);
      shuttdown_communication_channel();
      break;

#endif

    default:
      printf("unknown cmd\n");
      break;
    }
    remainlen -= cmd->len;
    pointer += cmd->len;
    if (remainlen < 2)
      break;
  }
}

void send_data_on_vhmi_channel(vhmi_cmd_t *msg) {
  esp_err_t err;
  switch (vhmi_call_type) {
  case 0:
    mqtt_publish_vhmi_data(msg->len, (uint8_t *)msg);
    break;
  case 1:
    err = tcpsocket_publish_vhmi_data(msg->len, (uint8_t *)msg);
    if (err != ESP_OK) {
      ESP_LOGW("Sockets", "err sending data on socket errno = %d", err);
      vhmi_call_type = 0; /// shift comm channel to mqtt
      mqtt_publish_vhmi_data(msg->len, (uint8_t *)msg);
    }
    break;
  case 2:
    err = senddataonble(msg->len, (uint8_t *)msg);
    if (err != ESP_OK) {
      ESP_LOGW("ble", "err sending data on ble errno = %d", err);
      vhmi_call_type = 0; /// shift comm channel to mqtt
      mqtt_publish_vhmi_data(msg->len, (uint8_t *)msg);
    }

    break;
  default:
    break;
  }
}

void shuttdown_communication_channel() {
  if (is_socket_connected) {
    print_logI("sockets", "Shutting down socket");
    tcpsocket_configure(0, 0, 0); // stop tcp socket
  }
  if (is_ble_connected) {
    print_logI("BLE", "Turning off BLE");
    Configure_ble(0); // turn off
  }
}