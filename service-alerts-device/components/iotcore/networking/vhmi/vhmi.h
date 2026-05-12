#ifndef _vhmi_h_
#define _vhmi_h_

#include "sdkconfig.h"
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef enum {
  vhmi_cmd_type_buttonpressed = 0x01,
  vhmi_cmd_type_wifiadd = 0x02,
  vhmi_cmd_type_Finger_detect = 0x03,
  vhmi_cmd_type_scanner_online_status = 0x04,
  vhmi_cmd_type_get_BLE_online_status =
      0x05, // command from app to on/off ble mode
  vhmi_cmd_type_set_BLE_online_status =
      0x06, // response from esp to app on online/offline ble status
  vhmi_cmd_type_set_socket_mode = 0x07,
  vhmi_cmd_type_socket_status = 0x08,
  vhmi_cmd_type_switch_channel = 0x09,

  vhmi_cmd_type_Add_user = 0x10,
  vhmi_cmd_type_Delete_Single_user = 0x11,
  vhmi_cmd_type_delete_all_users = 0x12,
  vhmi_cmd_type_delete_all_users_response = 0x13,
  vhmi_cmd_type_Enter_user_id = 0x14,
  vhmi_cmd_type_Enter_Password = 0x15,
  vhmi_cmd_type_User_added = 0x16,
  vhmi_cmd_type_online_stat_and_give_Password = 0x17,
  vhmi_cmd_type_Auth_status = 0x18,
  vhmi_cmd_type_Finger_place = 0x19,
  vhmi_cmd_type_Finger_remove = 0x1A,
  vhmi_cmd_type_scanner_error = 0x1B,
  vhmi_cmd_type_fingerprint_matched = 0x1C,
  vhmi_cmd_type_fingerprint_deleted = 0x1D,
  vhmi_cmd_type_EnterUsername = 0x1E,
  vhmi_cmd_type_Username = 0x1F,
  vhmi_cmd_type_Get_user = 0x20,
  vhmi_cmd_type_Get_number_of_user = 0x21,
  vhmi_cmd_type_Change_password = 0x22,
  vhmi_cmd_type_Password_changed = 0x23,
  vhmi_cmd_type_get_out_of_enroll = 0x24,
  vhmi_cmd_type_out_from_enroll = 0x25,
  vhmi_cmd_type_get_out_of_noOfusersMode = 0x26,
  vhmi_cmd_type_out_from_noOfusersMode = 0x27,
  vhmi_cmd_type_get_out_of_deleteAllusersMode = 0x28,
  vhmi_cmd_type_out_from_deleteAllusersMode = 0x29,
  vhmi_cmd_type_UserId = 0x2A,
  vhmi_cmd_type_Delete_User_response = 0x2B,
  vhmi_cmd_type_out_from_deleteSingleuserMode = 0x2C,
  vhmi_cmd_type_get_out_from_deleteSingleuserMode = 0x2D,
  vhmi_cmd_type_Enter_new_pass = 0x2E,
  vhmi_cmd_type_new_pass = 0x2F,
  vhmi_cmd_type_out_from_New_Pass_mode = 0x30,
  vhmi_cmd_type_get_out_from_New_Pass_mode = 0x31,

  vhmi_cmd_type_tank_parameter_settings = 0x40,
  vhmi_cmd_type_tank_parameter_settings_update_response = 0x41,

  vhmi_cmd_type_get_tank_parameter_settings = 0x42,

  vhmi_cmd_type_tank_parameter_settings_data_to_vhmi = 0x43,

  vhmi_cmd_type_shutdown_communication_channel = 0x48

} vhmi_cmd_type_t;

typedef struct __attribute__((__packed__)) {
  int8_t button; // number of button pressed
  int32_t pressedfor;
} vhmi_cmd_type_buttonpressed_t;

typedef struct __attribute__((__packed__)) {
  uint8_t Config; // number of button pressed
} vhmi_cmd_type_set_ble_t;

typedef struct __attribute__((__packed__)) {
  uint8_t status; // number of button pressed
} vhmi_cmd_tank_settings_update_response_t;

typedef struct __attribute__((__packed__)) {
  uint8_t set; // number of button pressed
  char BLE_name[32];

} vhmi_cmd_type_ble_status_t;

typedef struct __attribute__((__packed__)) {
  uint8_t Config;
  char wifi_ssid[50];
} vhmi_cmd_type_socket_mode_t;

typedef struct __attribute__((__packed__)) {
  uint8_t status;
  int32_t ip;
  uint16_t port;

} vhmi_cmd_type_socket_t;

typedef struct __attribute__((__packed__)) {
  uint8_t swtch; // 0 = mqtt
                 // 1 = socket
                 // 2 = ble
} vhmi_cmd_type_switch_comm_channel_t;

#ifdef CONFIG_FINGER_PRINT

typedef struct __attribute__((__packed__)) {
  uint32_t password;
} vhmi_cmd_type_Enter_password_t;

typedef struct __attribute__((__packed__)) {
  uint32_t password;
} vhmi_cmd_type_New_password_t;
typedef struct __attribute__((__packed__)) {
  int8_t status;
} vhmi_cmd_type_Auth_status_t;

typedef struct __attribute__((__packed__)) {
  uint8_t online_status;
} vhmi_cmd_type_scanner_online_status_t;
typedef struct __attribute__((__packed__)) {
  uint8_t status;
} vhmi_cmd_type_Finger_place_t;

typedef struct __attribute__((__packed__)) {
  uint8_t status;
} vhmi_cmd_type_Finger_remove_t;

typedef struct __attribute__((__packed__)) {
  uint8_t status;
} vhmi_cmd_type_Finger_detect_t;
typedef struct __attribute__((__packed__)) {
  uint8_t status;
} vhmi_cmd_type_prints_matched_t;

typedef struct __attribute__((__packed__)) {
  uint8_t status;
  uint16_t user_id;
} vhmi_cmd_type_user_add_t;

typedef struct __attribute__((__packed__)) {
  uint8_t status;
} vhmi_cmd_type_Scanner_error_t;

typedef struct __attribute__((__packed__)) {
  uint8_t status;
} vhmi_cmd_type_Out_from_enroll_t;
typedef struct __attribute__((__packed__)) {
  uint8_t status;
} vhmi_cmd_type_Out_from_getNoOfUsers_mode_t;

typedef struct __attribute__((__packed__)) {
  uint8_t status;
} vhmi_cmd_type_Out_deleteAllUsers_mode_t;

typedef struct __attribute__((__packed__)) {
  char user_name[33];
} vhmi_cmd_type_Username_t;

typedef struct __attribute__((__packed__)) {
  uint8_t status;
} vhmi_cmd_type_EnterUsername_t;

typedef struct __attribute__((__packed__)) {
  uint8_t status;
} vhmi_cmd_type_delete_all_users_t;

typedef struct __attribute__((__packed__)) {
  uint8_t status_bit;
  uint16_t no_of_users;

} vhmi_cmd_type_get_no_of_users_t;

typedef struct __attribute__((__packed__)) {
  uint16_t user_id;
} vhmi_cmd_type_UserId_to_del_t;

typedef struct __attribute__((__packed__)) {
  uint8_t status;
} vhmi_cmd_type_Enter_userId_t;
typedef struct __attribute__((__packed__)) {
  uint8_t status;
} vhmi_cmd_type_Enter_new_passwprd_t;
typedef struct __attribute__((__packed__)) {
  uint8_t status;
} vhmi_cmd_type_Delete_single_user_t;

typedef struct __attribute__((__packed__)) {
  uint8_t status;
} vhmi_cmd_type_Out_deleteSingleUsers_mode_t;
typedef struct __attribute__((__packed__)) {
  uint8_t status;
} vhmi_cmd_type_Out_Change_pass_mode_t;

typedef struct __attribute__((__packed__)) {
  uint8_t status;
} vhmi_cmd_type_Out_Change_pass_Response_mode_t;
#endif

#ifdef CONFIG_WATER_TANK_LEVEL

typedef struct __attribute__((__packed__)) {
  uint16_t tank_diameter_inches; /// tank diameter

} vhmi_cmd_type_cylindrical_tank_settings_t;

typedef struct __attribute__((__packed__)) {
  uint16_t tank_length_inches; /// tank length
  uint16_t tank_width_inches;  /// tank width

} vhmi_cmd_type_sqr_rec_tank_settings_t;

typedef struct __attribute__((__packed__)) {
  uint8_t tank_shape; /// square/rectangular -> value 1, cylindrical -> value 2
  uint8_t data_update_interval_min; /// data update to cloud interval
  uint8_t Motor_button_timeout_min; /// timeout to turn off motor if motor state
                                    /// switched from button

  uint16_t tank_height_inches;   /// tank height
  uint16_t Hmax_level_percent;   /// Max_level
  uint16_t Halert_level_percent; /// Alert_level
  uint16_t Hmin_level_percent;   /// Min_level

  union {
    vhmi_cmd_type_cylindrical_tank_settings_t cylindrical_tank;
    vhmi_cmd_type_sqr_rec_tank_settings_t Sqr_rect_tank;
  };

} vhmi_cmd_type_tank_parameter_settings_t;

typedef struct __attribute__((__packed__)) {
  uint16_t tank_diameter_inches; /// tank diameter

} vhmi_cmd_type_cylindrical_tank_settings_to_vhmi_t;

typedef struct __attribute__((__packed__)) {
  uint16_t tank_length_inches; /// tank length
  uint16_t tank_width_inches;  /// tank width

} vhmi_cmd_type_sqr_rec_tank_settings_to_vhmi_t;

typedef struct __attribute__((__packed__)) {
  uint8_t status;
  uint8_t tank_shape; /// square/rectangular -> value 1, cylindrical -> value 2
  uint8_t data_update_interval_min; /// data update to cloud interval
  uint8_t Motor_button_timeout_min; /// timeout to turn off motor if motor state
                                    /// switched from button

  uint16_t tank_height_inches;   /// tank height
  uint16_t Hmax_level_percent;   /// Max_level
  uint16_t Halert_level_percent; /// Alert_level
  uint16_t Hmin_level_percent;   /// Min_level

  union {
    vhmi_cmd_type_cylindrical_tank_settings_to_vhmi_t cylindrical_tank;
    vhmi_cmd_type_sqr_rec_tank_settings_to_vhmi_t Sqr_rect_tank;
  };

} vhmi_cmd_type_tank_parameter_settings_to_vhmi_t;
#endif

typedef struct __attribute__((__packed__)) {
  int8_t len;
  uint8_t cmd_type;
  union {
    uint8_t data[200];
    vhmi_cmd_type_buttonpressed_t buttonpressed;
    vhmi_cmd_type_set_ble_t Ble;
    vhmi_cmd_type_ble_status_t BleStatus;
    vhmi_cmd_type_socket_mode_t socket_mode;
    vhmi_cmd_type_socket_t Socket_data;
    vhmi_cmd_type_switch_comm_channel_t comm_channel;
#ifdef CONFIG_FINGER_PRINT
    vhmi_cmd_type_Auth_status_t Auth_status;
    vhmi_cmd_type_Enter_password_t Enter_password;
    vhmi_cmd_type_scanner_online_status_t Scanner_status;
    vhmi_cmd_type_Finger_place_t FingerPlace;
    vhmi_cmd_type_Finger_remove_t FingerRemove;
    vhmi_cmd_type_Finger_detect_t FingerDetect;
    vhmi_cmd_type_prints_matched_t FingerPrints_match;
    vhmi_cmd_type_user_add_t UserAdded;
    vhmi_cmd_type_Scanner_error_t ScannerError;
    vhmi_cmd_type_Out_from_enroll_t OutFromEnroll;
    vhmi_cmd_type_get_no_of_users_t GetNoOfUsers;
    vhmi_cmd_type_Out_from_getNoOfUsers_mode_t OutfromNoOfUsers;
    vhmi_cmd_type_delete_all_users_t DeleteAllUsers;
    vhmi_cmd_type_Out_deleteAllUsers_mode_t OutfromDeleteAllUsers;
    vhmi_cmd_type_Username_t User;
    vhmi_cmd_type_EnterUsername_t EnterUsername;
    vhmi_cmd_type_Enter_userId_t EnterUserId;
    vhmi_cmd_type_UserId_to_del_t UserToDel;
    vhmi_cmd_type_Delete_single_user_t DeleteUserResponse;
    vhmi_cmd_type_Out_deleteSingleUsers_mode_t OutfromDeleteSingleUser;
    vhmi_cmd_type_New_password_t New_password;
    vhmi_cmd_type_Enter_new_passwprd_t EnterNewPassword;
    vhmi_cmd_type_Out_Change_pass_mode_t OutfromChangePassword;
    vhmi_cmd_type_Out_Change_pass_Response_mode_t ChangePasswordResponse;
#endif
#ifdef CONFIG_WATER_TANK_LEVEL
    vhmi_cmd_type_tank_parameter_settings_t tank_parameters_settings;
    vhmi_cmd_tank_settings_update_response_t tank_settings_update;
    vhmi_cmd_type_tank_parameter_settings_to_vhmi_t
        tank_parameters_settings_to_vhmi;
#endif
  };

} vhmi_cmd_t;

void data_recived_mqtt(int32_t len, uint8_t *data);
void data_recived_socket(int32_t len, uint8_t *data);
void send_data_on_vhmi_channel(vhmi_cmd_t *msg);
void shuttdown_communication_channel();
void data_recived_on_ble(int32_t len, uint8_t *data);

#endif