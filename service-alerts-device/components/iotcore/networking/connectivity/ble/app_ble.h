/*
 * Licensed to the Apache Software Foundation (ASF) under one
 * or more contributor license agreements.  See the NOTICE file
 * distributed with this work for additional information
 * regarding copyright ownership.  The ASF licenses this file
 * to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance
 * with the License.  You may obtain a copy of the License at
 *
 *  http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing,
 * software distributed under the License is distributed on an
 * "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
 * KIND, either express or implied.  See the License for the
 * specific language governing permissions and limitations
 * under the License.
 */

#ifndef _app_ble_h_
#define _app_ble_h_

#include "esp_err.h"
#include "nimble/ble.h"
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

struct ble_hs_cfg;
struct ble_gatt_register_ctxt;

/** GATT server. */
#define GATT_SVR_SVC_ALERT_UUID 0x1811
#define GATT_SVR_CHR_SUP_NEW_ALERT_CAT_UUID 0x2A47
#define GATT_SVR_CHR_NEW_ALERT 0x2A46
#define GATT_SVR_CHR_SUP_UNR_ALERT_CAT_UUID 0x2A48
#define GATT_SVR_CHR_UNR_ALERT_STAT_UUID 0x2A45
#define GATT_SVR_CHR_ALERT_NOT_CTRL_PT 0x2A44

extern uint16_t vhmi_TX_handle;
extern bool notify_state;
extern uint16_t conn_handle;
extern uint8_t is_ble_connected;

typedef struct {
  void (*data_recived_cb)(int32_t, uint8_t *);
  char *bleName;
  int32_t password; // password for  bonding the ble connection

} app_ble_t;

/* Console */
int scli_init(void);
int scli_receive_key(int *key);

void ble_init(char *ble_name, uint32_t pass);
void app_ble(app_ble_t *config);
esp_err_t senddataonble(uint16_t size, uint8_t *data);
void app_ble_registercb_vhmi(void *cb_function);
void Stop_ble();

#ifdef __cplusplus
}
#endif

#endif
