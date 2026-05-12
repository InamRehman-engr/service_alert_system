
#ifndef __postToSlack_H__
#define __postToSlack_H__
#include "DC-codes.h"
#include "esp_err.h"
#include "iotcore_events.h"

extern int32_t GOODByProtocolInInit;

extern dc_codes_bits error_codes_dc;
extern dc_codes_bits error_codes_dc_reported;

#define post_to_slackarray_max 10
extern int32_t post_to_slackarray_count;
extern char *post_to_slackarray[];

extern esp_err_t http_postToSlack(char *pvParameters);
void http_posterror_set_uri(char *uri, const uint8_t *cert_pem);

/// TODO: Add watchdog triggered handling

void MY_ERROR_HANDLING_UARTError(char *error, int32_t clientID);
void MY_ERROR_HANDLING_SlaveDeviceError(char *error, int32_t clientID);
void MY_ERROR_HANDLING_INAError(char *error, int32_t clientID);
void MY_ERROR_HANDLING_INAoutOfRange(float vbat, float current,
                                     int32_t clientID);
void MY_ERROR_HANDLING_PostBattery(float voltage, int32_t clientID);
void MY_ERROR_HANDLING_PostDcERRORCode(int dc_code, int32_t clientID);

esp_err_t start_iotcore_error_handler(int32_t *clientID);
#endif