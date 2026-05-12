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

#include "app_ble.h"
#include "esp_err.h"
#include "esp_mac.h"
#include "esp_system.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
/* BLE */
#include "console/console.h"
#include "esp_nimble_hci.h"
#include "host/ble_hs.h"
#include "host/ble_uuid.h"
#include "host/util/util.h"
#include "nimble/nimble_port.h"
#include "nimble/nimble_port_freertos.h"
#include "services/gap/ble_svc_gap.h"
#include "services/gatt/ble_svc_gatt.h"
#include "vhmi.h"

#define TAG "APP_BLE"

#define DebugPrints
#define PRINT

#if defined(DebugPrints) && defined(PRINT)
#define print_d printf
#define printd_HEXDUMP ESP_LOG_BUFFER_HEXDUMP
#else
#define printd(...)
#define printd_HEXDUMP(...)
#endif
uint8_t is_ble_connected = 0;

static int bleprph_gap_event(struct ble_gap_event *event, void *arg);
static uint8_t own_addr_type;
static int32_t bonding_password = 123456;

static void (*data_recived_ble)(int32_t, uint8_t *) = NULL;

/**
 * The vendor specific security test service consists of two characteristics:
 *     o random-number-generator: generates a random 32-bit number each time
 *       it is read.  This characteristic can only be read over an encrypted
 *       connection.
 *     o static-value: a single-byte characteristic that can always be read,
 *       but can only be written over an encrypted connection.
 */

/* 59462f12-9543-9999-12c8-58b459a2712d */
static const ble_uuid128_t gatt_svr_svc_sec_vhmi_uuid =
    BLE_UUID128_INIT(0x2d, 0x71, 0xa2, 0x59, 0xb4, 0x58, 0xc8, 0x12, 0x99, 0x99,
                     0x43, 0x95, 0x12, 0x2f, 0x46, 0x59);

/* 5c3a659e-897e-45e1-b016-007107c96df6 */
static const ble_uuid128_t gatt_svr_chr_sec_TX_uuid =
    BLE_UUID128_INIT(0xf6, 0x6d, 0xc9, 0x07, 0x71, 0x00, 0x16, 0xb0, 0xe1, 0x45,
                     0x7e, 0x89, 0x9e, 0x65, 0x3a, 0x5c);

/* 5c3a659e-897e-45e1-b016-007107c96df7 */
static const ble_uuid128_t gatt_svr_chr_sec_RX_uuid =
    BLE_UUID128_INIT(0xf7, 0x6d, 0xc9, 0x07, 0x71, 0x00, 0x16, 0xb0, 0xe1, 0x45,
                     0x7e, 0x89, 0x9e, 0x65, 0x3a, 0x5c);

static uint8_t ble_last_Rx_vhmi_val[100];
static uint16_t ble_last_Rx_vhmi_len = 0;

static uint8_t ble_last_Tx_vhmi_val[100];
static uint16_t ble_last_Tx_vhmi_len = 0;

void print_addr(const void *addr) {
  const uint8_t *u8p;

  u8p = addr;
  printf("%02x:%02x:%02x:%02x:%02x:%02x", u8p[5], u8p[4], u8p[3], u8p[2],
         u8p[1], u8p[0]);
}

static int gatt_svr_chr_access_sec_test(uint16_t conn_handle,
                                        uint16_t attr_handle,
                                        struct ble_gatt_access_ctxt *ctxt,
                                        void *arg);

uint16_t vhmi_TX_handle;
bool notify_state = false;
uint16_t conn_handle;

static const struct ble_gatt_svc_def gatt_svr_svcs[] = {
    {
        /*** Service: Security test. */
        .type = BLE_GATT_SVC_TYPE_PRIMARY,
        .uuid = &gatt_svr_svc_sec_vhmi_uuid.u,
        .characteristics =
            (struct ble_gatt_chr_def[]){
                {
                    /*** Characteristic: Random number generator. */
                    .uuid = &gatt_svr_chr_sec_TX_uuid.u,
                    .access_cb = gatt_svr_chr_access_sec_test,
                    .flags = BLE_GATT_CHR_F_READ | BLE_GATT_CHR_F_NOTIFY |
                             BLE_GATT_CHR_F_READ_ENC,
                    .val_handle = &vhmi_TX_handle,
                },
                {
                    /*** Characteristic: Static value. */
                    .uuid = &gatt_svr_chr_sec_RX_uuid.u,
                    .access_cb = gatt_svr_chr_access_sec_test,
                    .flags = BLE_GATT_CHR_F_READ | BLE_GATT_CHR_F_WRITE |
                             BLE_GATT_CHR_F_WRITE_ENC,
                },
                {
                    0, /* No more characteristics in this service. */
                }},
    },

    {
        0, /* No more services. */
    },
};

static int gatt_svr_chr_write(struct os_mbuf *om, uint16_t min_len,
                              uint16_t max_len, void *dst, uint16_t *len) {
  uint16_t om_len;
  int rc;

  om_len = OS_MBUF_PKTLEN(om);
  ESP_LOGI("ble", "om_len %d", om_len);

  if (om_len < min_len || om_len > max_len) {
    return BLE_ATT_ERR_INVALID_ATTR_VALUE_LEN;
  }

  rc = ble_hs_mbuf_to_flat(om, dst, max_len, len);
  if (rc != 0) {
    return BLE_ATT_ERR_UNLIKELY;
  }
  ESP_LOGI("ble", "ble_hs_mbuf_to_flat flags%X", om->om_flags);

  return 0;
}

static int gatt_svr_chr_access_sec_test(uint16_t conn_handle,
                                        uint16_t attr_handle,
                                        struct ble_gatt_access_ctxt *ctxt,
                                        void *arg) {
  const ble_uuid_t *uuid;
  int rc;

  uuid = ctxt->chr->uuid;

  /* Determine which characteristic is being accessed by examining its
   * 128-bit UUID.
   */

  if (ble_uuid_cmp(uuid, &gatt_svr_chr_sec_TX_uuid.u) == 0) {
    ESP_LOGW("ble", "ble_uuid_cmpR");
    assert(ctxt->op == BLE_GATT_ACCESS_OP_READ_CHR);

    rc = os_mbuf_append(ctxt->om, ble_last_Rx_vhmi_val, ble_last_Rx_vhmi_len);
    return rc == 0 ? 0 : BLE_ATT_ERR_INSUFFICIENT_RES;
  }

  if (ble_uuid_cmp(uuid, &gatt_svr_chr_sec_RX_uuid.u) == 0) {
    switch (ctxt->op) {
    case BLE_GATT_ACCESS_OP_READ_CHR:
      ESP_LOGI("ble", "BLE_GATT_ACCESS_OP_READ_CHR");
      rc = os_mbuf_append(ctxt->om, ble_last_Tx_vhmi_val, ble_last_Tx_vhmi_len);
      return rc == 0 ? 0 : BLE_ATT_ERR_INSUFFICIENT_RES;

    case BLE_GATT_ACCESS_OP_WRITE_CHR:
      ESP_LOGI("ble", "BLE_GATT_ACCESS_OP_WRITE_CHR");
      rc = gatt_svr_chr_write(ctxt->om, 1, sizeof ble_last_Tx_vhmi_val,
                              ble_last_Tx_vhmi_val, &ble_last_Tx_vhmi_len);
      if (rc == 0 && data_recived_ble != NULL)
        data_recived_ble(ble_last_Tx_vhmi_len, ble_last_Tx_vhmi_val);
      return rc;
    case BLE_GATT_ACCESS_OP_READ_DSC:
      ESP_LOGI("ble", "BLE_GATT_ACCESS_OP_READ_DSC");
      return 0;
    case BLE_GATT_ACCESS_OP_WRITE_DSC:
      ESP_LOGI("ble", "BLE_GATT_ACCESS_OP_WRITE_DSC");
      return 0;
    default:
      assert(0);
      return BLE_ATT_ERR_UNLIKELY;
    }
  }

  /* Unknown characteristic; the nimble stack should not have called this
   * function.
   */
  assert(0);
  return BLE_ATT_ERR_UNLIKELY;
}

void gatt_svr_register_cb(struct ble_gatt_register_ctxt *ctxt, void *arg) {
  char buf[BLE_UUID_STR_LEN];

  switch (ctxt->op) {
  case BLE_GATT_REGISTER_OP_SVC:
    MODLOG_DFLT(DEBUG, "registered service %s with handle=%d\n",
                ble_uuid_to_str(ctxt->svc.svc_def->uuid, buf),
                ctxt->svc.handle);
    break;

  case BLE_GATT_REGISTER_OP_CHR:
    MODLOG_DFLT(DEBUG,
                "registering characteristic %s with "
                "def_handle=%d val_handle=%d\n",
                ble_uuid_to_str(ctxt->chr.chr_def->uuid, buf),
                ctxt->chr.def_handle, ctxt->chr.val_handle);
    break;

  case BLE_GATT_REGISTER_OP_DSC:
    MODLOG_DFLT(DEBUG, "registering descriptor %s with handle=%d\n",
                ble_uuid_to_str(ctxt->dsc.dsc_def->uuid, buf),
                ctxt->dsc.handle);
    break;

  default:
    assert(0);
    break;
  }
}

int gatt_svr_init(void) {
  int rc;

  ble_svc_gap_init();
  ble_svc_gatt_init();

  rc = ble_gatts_count_cfg(gatt_svr_svcs);
  if (rc != 0) {
    return rc;
  }

  rc = ble_gatts_add_svcs(gatt_svr_svcs);
  if (rc != 0) {
    return rc;
  }

  return 0;
}

/**
 * Logs information about a connection to the console.
 */
static void bleprph_print_conn_desc(struct ble_gap_conn_desc *desc) {
  ESP_LOGI(TAG,
           "handle=%d our_ota_addr_type=%d our_ota_addr=", desc->conn_handle,
           desc->our_ota_addr.type);
  print_addr(desc->our_ota_addr.val);
  ESP_LOGI(TAG, " our_id_addr_type=%d our_id_addr=", desc->our_id_addr.type);
  print_addr(desc->our_id_addr.val);
  ESP_LOGI(TAG,
           " peer_ota_addr_type=%d peer_ota_addr=", desc->peer_ota_addr.type);
  print_addr(desc->peer_ota_addr.val);
  ESP_LOGI(TAG, " peer_id_addr_type=%d peer_id_addr=", desc->peer_id_addr.type);
  print_addr(desc->peer_id_addr.val);
  ESP_LOGI(TAG,
           " conn_itvl=%d conn_latency=%d supervision_timeout=%d "
           "encrypted=%d authenticated=%d bonded=%d\n",
           desc->conn_itvl, desc->conn_latency, desc->supervision_timeout,
           desc->sec_state.encrypted, desc->sec_state.authenticated,
           desc->sec_state.bonded);
}

/**
 * Enables advertising with the following parameters:
 *     o General discoverable mode.
 *     o Undirected connectable mode.
 */
static void bleprph_advertise(void) {
  struct ble_gap_adv_params adv_params;
  struct ble_hs_adv_fields fields;
  const char *name;
  int rc;

  /**
   *  Set the advertisement data included in our advertisements:
   *     o Flags (indicates advertisement type and other general info).
   *     o Advertising tx power.
   *     o Device name.
   *     o 16-bit service UUIDs (alert notifications).
   */

  memset(&fields, 0, sizeof fields);

  /* Advertise two flags:
   *     o Discoverability in forthcoming advertisement (general)
   *     o BLE-only (BR/EDR unsupported).
   */
  fields.flags = BLE_HS_ADV_F_DISC_GEN | BLE_HS_ADV_F_BREDR_UNSUP;

  /* Indicate that the TX power level field should be included; have the
   * stack fill this value automatically.  This is done by assigning the
   * special value BLE_HS_ADV_TX_PWR_LVL_AUTO.
   */
  fields.tx_pwr_lvl_is_present = 1;
  fields.tx_pwr_lvl = BLE_HS_ADV_TX_PWR_LVL_AUTO;

  name = ble_svc_gap_device_name();
  fields.name = (uint8_t *)name;
  fields.name_len = strlen(name);
  fields.name_is_complete = 1;

  fields.uuids16 = (ble_uuid16_t[]){BLE_UUID16_INIT(GATT_SVR_SVC_ALERT_UUID)};
  fields.num_uuids16 = 1;
  fields.uuids16_is_complete = 1;

  rc = ble_gap_adv_set_fields(&fields);
  if (rc != 0) {
    MODLOG_DFLT(ERROR, "error setting advertisement data; rc=%d\n", rc);
    return;
  }

  /* Begin advertising. */
  memset(&adv_params, 0, sizeof adv_params);
  adv_params.conn_mode = BLE_GAP_CONN_MODE_UND;
  adv_params.disc_mode = BLE_GAP_DISC_MODE_GEN;
  rc = ble_gap_adv_start(own_addr_type, NULL, BLE_HS_FOREVER, &adv_params,
                         bleprph_gap_event, NULL);
  if (rc != 0) {
    MODLOG_DFLT(ERROR, "error enabling advertisement; rc=%d\n", rc);
    return;
  }
}

/**
 * The nimble host executes this callback when a GAP event occurs.  The
 * application associates a GAP event callback with each connection that forms.
 * bleprph uses the same callback for all connections.
 *
 * @param event                 The type of event being signalled.
 * @param ctxt                  Various information pertaining to the event.
 * @param arg                   Application-specified argument; unused by
 *                                  bleprph.
 *
 * @return                      0 if the application successfully handled the
 *                                  event; nonzero on failure.  The semantics
 *                                  of the return code is specific to the
 *                                  particular GAP event being signalled.
 */
static int bleprph_gap_event(struct ble_gap_event *event, void *arg) {
  struct ble_gap_conn_desc desc;
  int rc;

  switch (event->type) {
  case BLE_GAP_EVENT_CONNECT:
    /* A new connection was established or a connection attempt failed. */
    ESP_LOGI(TAG, "connection %s; status=%d ",
             event->connect.status == 0 ? "established" : "failed",
             event->connect.status);
    if (event->connect.status == 0) {
      rc = ble_gap_conn_find(event->connect.conn_handle, &desc);
      assert(rc == 0);
      bleprph_print_conn_desc(&desc);
      conn_handle = event->connect.conn_handle;
    }
    ESP_LOGI(TAG, "\n");

    if (event->connect.status != 0) {
      /* Connection failed; resume advertising. */
      bleprph_advertise();
    }
    return 0;

  case BLE_GAP_EVENT_DISCONNECT:
    ESP_LOGI(TAG, "disconnect; reason=%d ", event->disconnect.reason);
    bleprph_print_conn_desc(&event->disconnect.conn);
    ESP_LOGI(TAG, "\n");

    /* Connection terminated; resume advertising. */
    bleprph_advertise();
    return 0;

  case BLE_GAP_EVENT_CONN_UPDATE:
    /* The central has updated the connection parameters. */
    ESP_LOGI(TAG, "connection updated; status=%d ", event->conn_update.status);
    rc = ble_gap_conn_find(event->connect.conn_handle, &desc);
    assert(rc == 0);
    bleprph_print_conn_desc(&desc);
    ESP_LOGI(TAG, "\n");
    return 0;

  case BLE_GAP_EVENT_ADV_COMPLETE:
    ESP_LOGI(TAG, "advertise complete; reason=%d", event->adv_complete.reason);
    bleprph_advertise();
    return 0;

  case BLE_GAP_EVENT_ENC_CHANGE:
    /* Encryption has been enabled or disabled for this connection. */
    ESP_LOGI(TAG, "encryption change event; status=%d ",
             event->enc_change.status);
    rc = ble_gap_conn_find(event->connect.conn_handle, &desc);
    assert(rc == 0);
    bleprph_print_conn_desc(&desc);
    ESP_LOGI(TAG, "\n");
    return 0;

  case BLE_GAP_EVENT_SUBSCRIBE:
    ESP_LOGI(TAG,
             "subscribe event; conn_handle=%d attr_handle=%d "
             "reason=%d prevn=%d curn=%d previ=%d curi=%d\n",
             event->subscribe.conn_handle, event->subscribe.attr_handle,
             event->subscribe.reason, event->subscribe.prev_notify,
             event->subscribe.cur_notify, event->subscribe.prev_indicate,
             event->subscribe.cur_indicate);

    if (event->subscribe.attr_handle == vhmi_TX_handle) {
      ESP_LOGW(TAG, "vhmi_TX_handle  %d", event->subscribe.cur_notify);
      notify_state = event->subscribe.cur_notify;
    } else if (event->subscribe.attr_handle != vhmi_TX_handle) {
      ESP_LOGE(TAG, "vhmi_TX_handle is not this why is this %d",
               event->subscribe.cur_notify);
      // notify_state = event->subscribe.cur_notify;
    }
    return 0;

  case BLE_GAP_EVENT_MTU:
    ESP_LOGI(TAG, "mtu update event; conn_handle=%d cid=%d mtu=%d\n",
             event->mtu.conn_handle, event->mtu.channel_id, event->mtu.value);
    return 0;

  case BLE_GAP_EVENT_REPEAT_PAIRING:
    /* We already have a bond with the peer, but it is attempting to
     * establish a new secure link.  This app sacrifices security for
     * convenience: just throw away the old bond and accept the new link.
     */

    /* Delete the old bond. */
    ESP_LOGI(TAG, "Delete the old bond \n");
    rc = ble_gap_conn_find(event->repeat_pairing.conn_handle, &desc);
    assert(rc == 0);
    ble_store_util_delete_peer(&desc.peer_id_addr);

    /* Return BLE_GAP_REPEAT_PAIRING_RETRY to indicate that the host should
     * continue with the pairing operation.
     */
    return BLE_GAP_REPEAT_PAIRING_RETRY;

  case BLE_GAP_EVENT_PASSKEY_ACTION:
    ESP_LOGI(TAG, "PASSKEY_ACTION_EVENT started \n");
    struct ble_sm_io pkey = {0};
    int key = 0;

    if (event->passkey.params.action == BLE_SM_IOACT_DISP) {
      pkey.action = event->passkey.params.action;
      pkey.passkey =
          bonding_password; // This is the passkey to be entered on peer
      ESP_LOGI(TAG, "Enter passkey %lu on the peer side", pkey.passkey);
      rc = ble_sm_inject_io(event->passkey.conn_handle, &pkey);
      ESP_LOGI(TAG, "ble_sm_inject_io result: %d\n", rc);
    } else if (event->passkey.params.action == BLE_SM_IOACT_NUMCMP) {
      ESP_LOGI(TAG, "Passkey on device's display: %lu",
               event->passkey.params.numcmp);
      ESP_LOGI(TAG, "Accept or reject the passkey through console in this "
                    "format -> key Y or key N");
      pkey.action = event->passkey.params.action;
      if (scli_receive_key(&key)) {
        pkey.numcmp_accept = key;
      } else {
        pkey.numcmp_accept = 0;
        ESP_LOGE(TAG, "Timeout! Rejecting the key");
      }
      rc = ble_sm_inject_io(event->passkey.conn_handle, &pkey);
      ESP_LOGI(TAG, "ble_sm_inject_io result: %d\n", rc);
    } else if (event->passkey.params.action == BLE_SM_IOACT_OOB) {
      static uint8_t tem_oob[16] = {0};
      pkey.action = event->passkey.params.action;
      for (int i = 0; i < 16; i++) {
        pkey.oob[i] = tem_oob[i];
      }
      rc = ble_sm_inject_io(event->passkey.conn_handle, &pkey);
      ESP_LOGI(TAG, "ble_sm_inject_io result: %d\n", rc);
    } else if (event->passkey.params.action == BLE_SM_IOACT_INPUT) {
      ESP_LOGI(TAG,
               "Enter the passkey through console in this format-> key 123456");
      pkey.action = event->passkey.params.action;
      if (scli_receive_key(&key)) {
        pkey.passkey = key;
      } else {
        pkey.passkey = 0;
        ESP_LOGE(TAG, "Timeout! Passing 0 as the key");
      }
      rc = ble_sm_inject_io(event->passkey.conn_handle, &pkey);
      ESP_LOGI(TAG, "ble_sm_inject_io result: %d\n", rc);
    }
    return 0;
  }

  return 0;
}

static void bleprph_on_reset(int reason) {
  MODLOG_DFLT(ERROR, "Resetting state; reason=%d\n", reason);
}

static void bleprph_on_sync(void) {
  int rc;

  rc = ble_hs_util_ensure_addr(0);
  assert(rc == 0);

  /* Figure out address to use while advertising (no privacy for now) */
  rc = ble_hs_id_infer_auto(0, &own_addr_type);
  if (rc != 0) {
    MODLOG_DFLT(ERROR, "error determining address type; rc=%d\n", rc);
    return;
  }

  /* Printing ADDR */
  uint8_t addr_val[6] = {0};
  rc = ble_hs_id_copy_addr(own_addr_type, addr_val, NULL);

  ESP_LOGI(TAG, "Device Address: ");
  print_addr(addr_val);
  ESP_LOGI(TAG, "\n");
  /* Begin advertising. */
  bleprph_advertise();
}

int bleprph_host_task_State = false;
void bleprph_host_task(void *param) {
  ESP_LOGI(TAG, "BLE Host Task Started");
  /* This function will return only when nimble_port_stop() is executed */
  bleprph_host_task_State = true;
  nimble_port_run();
  ESP_LOGW(TAG, "BLE Host Task ended");
  bleprph_host_task_State = false;

  nimble_port_freertos_deinit();
}

esp_err_t senddataonble(uint16_t size, uint8_t *data) {
  esp_err_t ret = ESP_FAIL;
  int rc;

  struct os_mbuf *om;
  ble_last_Rx_vhmi_len = size;
  memcpy(ble_last_Rx_vhmi_val, data, size);
  ESP_LOG_BUFFER_HEXDUMP(TAG, data, size, ESP_LOG_INFO);
  if (notify_state) {
    om = ble_hs_mbuf_from_flat(data, size);
    rc = ble_gattc_notify_custom(conn_handle, vhmi_TX_handle, om);
    if (rc == 0)
      ret = ESP_OK;
  } else {
    ESP_LOGI(TAG, "notify_state is off");
  }

  return ret;
}
void app_ble_registercb_vhmi(void *cb_function) {
  data_recived_ble = cb_function;
}

void Stop_ble() {

  printf("\nStop_ble  .. %d\n", is_ble_connected);
  if (is_ble_connected) {
    is_ble_connected = 0;

    int ret = nimble_port_stop();
    printf("\nStop_ble  ret = %d\n", ret);
    if (ret == 0) {

      printf("\nin nimble_port_deinit\n");
      nimble_port_deinit();
      while (bleprph_host_task_State) {
        printf("\nbleprph_host_task_State is active currently\n");
        vTaskDelay(100 / portTICK_PERIOD_MS);
      }

      printf("\nBle disconnected\n");
    }
  }
}

void app_ble(app_ble_t *config) {
  int rc;
  uint8_t MAC_BLE[8];
  char BLE_NAME[32];
  if (config != NULL) {
    if (config->bleName) {
      strcpy(BLE_NAME, config->bleName);
    } else {
      esp_read_mac(MAC_BLE, ESP_MAC_BT);
      sprintf((char *)BLE_NAME, "ESP_%02X%02X%02X", MAC_BLE[3], MAC_BLE[4],
              MAC_BLE[5]);
    }
    bonding_password = config->password;
    if (config->data_recived_cb != NULL)
      app_ble_registercb_vhmi(config->data_recived_cb);
  }

  ///    ESP_ERROR_CHECK(esp_nimble_hci_and_controller_init());

  int ret = esp_nimble_hci_init();
  if (ret != ESP_OK) {
    ESP_LOGE(TAG, "esp_nimble_hci_and_controller_init() failed with error: %d",
             ret);
    ret = esp_nimble_hci_init();
    if (ret != ESP_OK) {
      return;
    }
  }

  nimble_port_init();
  /* Initialize the NimBLE host configuration. */
  ble_hs_cfg.reset_cb = bleprph_on_reset;
  ble_hs_cfg.sync_cb = bleprph_on_sync;
  ble_hs_cfg.gatts_register_cb = gatt_svr_register_cb;
  ble_hs_cfg.store_status_cb = ble_store_util_status_rr;

  ble_hs_cfg.sm_io_cap = CONFIG_EXAMPLE_IO_TYPE;
#ifdef CONFIG_EXAMPLE_BONDING
  ble_hs_cfg.sm_bonding = 1;
#endif
#ifdef CONFIG_EXAMPLE_MITM
  ble_hs_cfg.sm_mitm = 1;
#endif
#ifdef CONFIG_EXAMPLE_USE_SC
  ble_hs_cfg.sm_sc = 1;
#else
  ble_hs_cfg.sm_sc = 0;
#ifdef CONFIG_EXAMPLE_BONDING
  ble_hs_cfg.sm_our_key_dist = 1;
  ble_hs_cfg.sm_their_key_dist = 1;
#endif
#endif

  rc = gatt_svr_init();
  assert(rc == 0);

  /* Set the default device name. */
  rc = ble_svc_gap_device_name_set((char *)BLE_NAME);
  assert(rc == 0);

  nimble_port_freertos_init(bleprph_host_task);

  /* Initialize command line interface to accept input from user */
  // rc = scli_init();
  // if (rc != ESP_OK) {
  //     ESP_LOGE(TAG, "scli_init() failed");
  // }

  vTaskDelay(500 / portTICK_PERIOD_MS);

  is_ble_connected = 1;
  /// BLE online status
}

void Ble_Online_Status(uint8_t status) {
  uint8_t MAC_BLE[8];
  char BLE_NAME[32];
  vhmi_cmd_t msg;

  esp_read_mac(MAC_BLE, ESP_MAC_BT);

  sprintf((char *)msg.BleStatus.BLE_name, "ESP1_%02X%02X%02X", MAC_BLE[3],
          MAC_BLE[4], MAC_BLE[5]);

  msg.cmd_type = vhmi_cmd_type_set_BLE_online_status;
  msg.BleStatus.set = (uint8_t)status;
  // strncpy(msg.BleStatus.BLE_name,BLE_NAME,strlen(BLE_NAME));
  // msg.BleStatus.BLE_name = BLE_NAME;
  msg.len = 3 + strlen(msg.BleStatus.BLE_name);

  //   print_d("\nlen = %d\n",msg.len);

  //       print_d("\nBLE NAME = %s\n",(char*)msg.BleStatus.BLE_name);

  send_data_on_vhmi_channel(&msg);
}
void Configure_ble(int set_ble) {
  print_d("\nset_ble = %d\n", set_ble);
  uint8_t MAC_BLE[8];
  char BLE_NAME[32];

  esp_read_mac(MAC_BLE, ESP_MAC_BT);

  if (set_ble == 1) {

    if (!is_ble_connected) {
      print_d("turning on ble\n");
      sprintf((char *)BLE_NAME, "ESP1_%02X%02X%02X", MAC_BLE[3], MAC_BLE[4],
              MAC_BLE[5]);
      // set a call back for ble
      app_ble_t ble_config = {
          .data_recived_cb = data_recived_on_ble,
          .bleName = BLE_NAME,
          .password = 123455,
      };
      app_ble(&ble_config);
    }

    Ble_Online_Status(is_ble_connected); /// BLE is ON
  } else {
    Ble_Online_Status(is_ble_connected);
    print_d("turning off ble\n");
    if (is_ble_connected)
      Stop_ble();
  }
}

void ble_init(char *ble_name, uint32_t pass) {
  app_ble_t ble_config = {
      .bleName = ble_name,
      .password = pass,
  };
  app_ble(&ble_config);
}