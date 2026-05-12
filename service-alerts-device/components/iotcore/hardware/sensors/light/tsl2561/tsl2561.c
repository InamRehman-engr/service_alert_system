#include "tsl2561.h"

static const char *TAG = "tsl2561";

#define WRITE_BIT I2C_MASTER_WRITE
#define READ_BIT I2C_MASTER_READ
#define ACK_CHECK true
#define NO_ACK_CHECK false
#define ACK_VALUE 0x0
#define NACK_VALUE 0x1
#define MAX_BLOCK_LEN 255 // SMBus v3.0 increases this from 32 to 255
// #define MEASURE             // enable measurement and reporting of I2C
// transaction duration

#define I2C_MASTER_NUM I2C_NUM_0
#define I2C_MASTER_TX_BUF_LEN 0 // disabled
#define I2C_MASTER_RX_BUF_LEN 0 // disabled
#define I2C_MASTER_FREQ_HZ 100000
#define I2C_MASTER_SDA_IO CONFIG_I2C_MASTER_SDA
#define I2C_MASTER_SCL_IO CONFIG_I2C_MASTER_SCL

// Register addresses
#define REG_CONTROL 0x00
#define REG_TIMING 0x01
#define REG_THRESHLOWLOW 0x02
#define REG_THRESHLOWHIGH 0x03
#define REG_THRESHHIGHLOW 0x04
#define REG_THRESHHIGHHIGH 0x05
#define REG_INTERRUPT 0x06
#define REG_ID 0x0A
#define REG_DATA0LOW 0x0C
#define REG_DATA0HIGH 0x0D
#define REG_DATA1LOW 0x0E
#define REG_DATA1HIGH 0x0F

// The following values are bitwise ORed with register addresses to create a
// command value
#define SMB_BLOCK 0x10   // Transaction to use Block Write/Read protocol
#define SMB_WORD 0x20    // Transaction to use Word Write/Read protocol
#define SMB_CLEAR 0x40   // Clear any pending interrupt (self-clearing)
#define SMB_COMMAND 0x80 // Select command register

#define TSL2561_CONTROL_POWER_UP 0x03
#define TSL2561_CONTROL_POWER_DOWN 0x00

// Device defaults:
#define DEFAULT_INTEGRATION_TIME TSL2561_INTEGRATION_TIME_402MS
#define DEFAULT_GAIN TSL2561_GAIN_1X

#define CH_SCALE 10           // Scale channel values by 2^10
#define CH_SCALE_TINT0 0x7517 // 322/11 * 2^CH_SCALE
#define CH_SCALE_TINT1 0x0FE7 // 322/81 * 2^CH_SCALE

#define RATIO_SCALE 9 // Scale ratio by 2^9
#define LUX_SCALE 14  // Scale by 2^14

// T, FN, and CL Package coefficients
#define TSL2561_K1T 0x0040
#define TSL2561_B1T 0x01F2
#define TSL2561_M1T 0x01BE
#define TSL2561_K2T 0x0080
#define TSL2561_B2T 0x0214
#define TSL2561_M2T 0x02D1
#define TSL2561_K3T 0x00C0
#define TSL2561_B3T 0x023F
#define TSL2561_M3T 0x037B
#define TSL2561_K4T 0x0100
#define TSL2561_B4T 0x0270
#define TSL2561_M4T 0x03FE
#define TSL2561_K5T 0x0138
#define TSL2561_B5T 0x016F
#define TSL2561_M5T 0x01fC
#define TSL2561_K6T 0x019A
#define TSL2561_B6T 0x00D2
#define TSL2561_M6T 0x00FB
#define TSL2561_K7T 0x029A
#define TSL2561_B7T 0x0018
#define TSL2561_M7T 0x0012
#define TSL2561_K8T 0x029A
#define TSL2561_B8T 0x0000
#define TSL2561_M8T 0x0000

static bool _is_smbus_init(const smbus_info_t *smbus_info) {
  bool ok = false;
  if (smbus_info != NULL) {
    if (smbus_info->init) {
      ok = true;
    } else {
      ESP_LOGE(TAG, "smbus_info is not initialised");
    }
  } else {
    ESP_LOGE(TAG, "smbus_info is NULL");
  }
  return ok;
}

static esp_err_t _check_i2c_error(esp_err_t err) {
  switch (err) {
  case ESP_OK: // Success
    break;
  case ESP_ERR_INVALID_ARG: // Parameter error
    ESP_LOGE(TAG, "I2C parameter error");
    break;
  case ESP_FAIL: // Sending command error, slave doesn't ACK the transfer.
    ESP_LOGE(TAG, "I2C no slave ACK");
    break;
  case ESP_ERR_INVALID_STATE: // I2C driver not installed or not in master mode.
    ESP_LOGE(TAG, "I2C driver not installed or not master");
    break;
  case ESP_ERR_TIMEOUT: // Operation timeout because the bus is busy.
    ESP_LOGE(TAG, "I2C timeout");
    break;
  default:
    ESP_LOGE(TAG, "I2C error %d", err);
  }
  return err;
}

esp_err_t _write_bytes(const smbus_info_t *smbus_info, uint8_t command,
                       uint8_t *data, size_t len) {
  // Protocol: [S | ADDR | Wr | As | COMMAND | As | (DATA | As){*len} | P]
  esp_err_t err = ESP_FAIL;
  if (_is_smbus_init(smbus_info) && data) {
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, smbus_info->address << 1 | WRITE_BIT, ACK_CHECK);
    i2c_master_write_byte(cmd, command, ACK_CHECK);
    i2c_master_write(cmd, data, len, ACK_CHECK);
    i2c_master_stop(cmd);
#ifdef MEASURE
    uint64_t start_time = esp_timer_get_time();
#endif
    err = _check_i2c_error(
        i2c_master_cmd_begin(smbus_info->i2c_port, cmd, smbus_info->timeout));
#ifdef MEASURE
    ESP_LOGI(TAG, "_write_bytes: i2c_master_cmd_begin took %" PRIu64 " us",
             esp_timer_get_time() - start_time);
#endif
    i2c_cmd_link_delete(cmd);
  }
  return err;
}

esp_err_t _read_bytes(const smbus_info_t *smbus_info, uint8_t command,
                      uint8_t *data, size_t len) {
  // Protocol: [S | ADDR | Wr | As | COMMAND | As | Sr | ADDR | Rd | As | (DATAs
  // | A){*len-1} | DATAs | N | P]
  esp_err_t err = ESP_FAIL;
  if (_is_smbus_init(smbus_info) && data) {
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, smbus_info->address << 1 | WRITE_BIT, ACK_CHECK);
    i2c_master_write_byte(cmd, command, ACK_CHECK);
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, smbus_info->address << 1 | READ_BIT, ACK_CHECK);
    if (len > 1) {
      i2c_master_read(cmd, data, len - 1, ACK_VALUE);
    }
    i2c_master_read_byte(cmd, &data[len - 1], NACK_VALUE);
    i2c_master_stop(cmd);
#ifdef MEASURE
    uint64_t start_time = esp_timer_get_time();
#endif
    err = _check_i2c_error(
        i2c_master_cmd_begin(smbus_info->i2c_port, cmd, smbus_info->timeout));
#ifdef MEASURE
    ESP_LOGI(TAG, "_read_bytes: i2c_master_cmd_begin took %" PRIu64 " us",
             esp_timer_get_time() - start_time);
#endif
    i2c_cmd_link_delete(cmd);
  }
  return err;
}

// Public API

smbus_info_t *smbus_malloc(void) {
  smbus_info_t *smbus_info = malloc(sizeof(*smbus_info));
  if (smbus_info != NULL) {
    memset(smbus_info, 0, sizeof(*smbus_info));
    ESP_LOGD(TAG, "malloc smbus_info_t %p", smbus_info);
  } else {
    ESP_LOGE(TAG, "malloc smbus_info_t failed");
  }
  return smbus_info;
}

void smbus_free(smbus_info_t **smbus_info) {
  if (smbus_info != NULL && (*smbus_info != NULL)) {
    ESP_LOGD(TAG, "free smbus_info_t %p", *smbus_info);
    free(*smbus_info);
    *smbus_info = NULL;
  } else {
    ESP_LOGE(TAG, "free smbus_info_t failed");
  }
}

esp_err_t smbus_init(smbus_info_t *smbus_info, i2c_port_t i2c_port,
                     i2c_address_t address) {
  if (smbus_info != NULL) {
    smbus_info->i2c_port = i2c_port;
    smbus_info->address = address;
    smbus_info->timeout = SMBUS_DEFAULT_TIMEOUT;
    smbus_info->init = true;
  } else {
    ESP_LOGE(TAG, "smbus_info is NULL");
    return ESP_FAIL;
  }
  return ESP_OK;
}

esp_err_t smbus_set_timeout(smbus_info_t *smbus_info, portBASE_TYPE timeout) {
  esp_err_t err = ESP_FAIL;
  if (_is_smbus_init(smbus_info)) {
    smbus_info->timeout = timeout;
    err = ESP_OK;
  }
  return err;
}

esp_err_t smbus_quick(const smbus_info_t *smbus_info, bool bit) {
  // Protocol: [S | ADDR | R/W | As | P]
  esp_err_t err = ESP_FAIL;
  if (_is_smbus_init(smbus_info)) {
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, smbus_info->address << 1 | bit, ACK_CHECK);
    i2c_master_stop(cmd);
    err = _check_i2c_error(
        i2c_master_cmd_begin(smbus_info->i2c_port, cmd, smbus_info->timeout));
    i2c_cmd_link_delete(cmd);
  }
  return err;
}

esp_err_t smbus_send_byte(const smbus_info_t *smbus_info, uint8_t data) {
  // Protocol: [S | ADDR | Wr | As | DATA | As | P]
  esp_err_t err = ESP_FAIL;
  if (_is_smbus_init(smbus_info)) {
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, smbus_info->address << 1 | WRITE_BIT, ACK_CHECK);
    i2c_master_write_byte(cmd, data, ACK_CHECK);
    i2c_master_stop(cmd);
    err = _check_i2c_error(
        i2c_master_cmd_begin(smbus_info->i2c_port, cmd, smbus_info->timeout));
    i2c_cmd_link_delete(cmd);
  }
  return err;
}

esp_err_t smbus_receive_byte(const smbus_info_t *smbus_info, uint8_t *data) {
  // Protocol: [S | ADDR | Rd | As | DATAs | N | P]
  esp_err_t err = ESP_FAIL;
  if (_is_smbus_init(smbus_info)) {
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, smbus_info->address << 1 | READ_BIT, ACK_CHECK);
    i2c_master_read_byte(cmd, data, NACK_VALUE);
    i2c_master_stop(cmd);
    err = _check_i2c_error(
        i2c_master_cmd_begin(smbus_info->i2c_port, cmd, smbus_info->timeout));
    i2c_cmd_link_delete(cmd);
  }
  return err;
}

esp_err_t smbus_write_byte(const smbus_info_t *smbus_info, uint8_t command,
                           uint8_t data) {
  // Protocol: [S | ADDR | Wr | As | COMMAND | As | DATA | As | P]
  return _write_bytes(smbus_info, command, &data, 1);
}

esp_err_t smbus_write_word(const smbus_info_t *smbus_info, uint8_t command,
                           uint16_t data) {
  // Protocol: [S | ADDR | Wr | As | COMMAND | As | DATA-LOW | As | DATA-HIGH |
  // As | P]
  uint8_t temp[2] = {data & 0xff, (data >> 8) & 0xff};
  return _write_bytes(smbus_info, command, temp, 2);
}

esp_err_t smbus_read_byte(const smbus_info_t *smbus_info, uint8_t command,
                          uint8_t *data) {
  // Protocol: [S | ADDR | Wr | As | COMMAND | As | Sr | ADDR | Rd | As | DATA |
  // N | P]
  return _read_bytes(smbus_info, command, data, 1);
}

esp_err_t smbus_read_word(const smbus_info_t *smbus_info, uint8_t command,
                          uint16_t *data) {
  // Protocol: [S | ADDR | Wr | As | COMMAND | As | Sr | ADDR | Rd | As |
  // DATA-LOW | A | DATA-HIGH | N | P]
  esp_err_t err = ESP_FAIL;
  uint8_t temp[2] = {0};
  if (data) {
    err = _read_bytes(smbus_info, command, temp, 2);
    if (err == ESP_OK) {
      *data = (temp[1] << 8) + temp[0];
    } else {
      *data = 0;
    }
  }
  return err;
}

esp_err_t smbus_write_block(const smbus_info_t *smbus_info, uint8_t command,
                            uint8_t *data, uint8_t len) {
  // Protocol: [S | ADDR | Wr | As | COMMAND | As | LEN | As | DATA-1 | As |
  // DATA-2 | As ... | DATA-LEN | As | P]
  esp_err_t err = ESP_FAIL;
  if (_is_smbus_init(smbus_info) && data) {
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, smbus_info->address << 1 | WRITE_BIT, ACK_CHECK);
    i2c_master_write_byte(cmd, command, ACK_CHECK);
    i2c_master_write_byte(cmd, len, ACK_CHECK);
    for (size_t i = 0; i < len; ++i) {
      i2c_master_write_byte(cmd, data[i], ACK_CHECK);
    }
    i2c_master_stop(cmd);
    err = _check_i2c_error(
        i2c_master_cmd_begin(smbus_info->i2c_port, cmd, smbus_info->timeout));
    i2c_cmd_link_delete(cmd);
  }
  return err;
}

esp_err_t smbus_read_block(const smbus_info_t *smbus_info, uint8_t command,
                           uint8_t *data, uint8_t *len) {
  // Protocol: [S | ADDR | Wr | As | COMMAND | As | Sr | ADDR | Rd | As | LENs |
  // A | DATA-1 | A | DATA-2 | A ... | DATA-LEN | N | P]
  esp_err_t err = ESP_FAIL;

  if (_is_smbus_init(smbus_info) && data && len) {
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, smbus_info->address << 1 | WRITE_BIT, ACK_CHECK);
    i2c_master_write_byte(cmd, command, ACK_CHECK);
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, smbus_info->address << 1 | READ_BIT, ACK_CHECK);
    uint8_t slave_len = 0;
    i2c_master_read_byte(cmd, &slave_len, ACK_VALUE);
    err = _check_i2c_error(
        i2c_master_cmd_begin(smbus_info->i2c_port, cmd, smbus_info->timeout));
    i2c_cmd_link_delete(cmd);

    if (err != ESP_OK) {
      *len = 0;
      return err;
    }

    if (slave_len > *len) {
      ESP_LOGW(TAG, "slave data length %d exceeds data len %d bytes", slave_len,
               *len);
      slave_len = *len;
    }

    cmd = i2c_cmd_link_create();
    for (size_t i = 0; i < slave_len - 1; ++i) {
      i2c_master_read_byte(cmd, &data[i], ACK_VALUE);
    }
    i2c_master_read_byte(cmd, &data[slave_len - 1], NACK_VALUE);
    i2c_master_stop(cmd);
    err = _check_i2c_error(
        i2c_master_cmd_begin(smbus_info->i2c_port, cmd, smbus_info->timeout));
    i2c_cmd_link_delete(cmd);

    if (err == ESP_OK) {
      *len = slave_len;
    } else {
      *len = 0;
    }
  }
  return err;
}

esp_err_t smbus_i2c_write_block(const smbus_info_t *smbus_info, uint8_t command,
                                uint8_t *data, size_t len) {
  // Protocol: [S | ADDR | Wr | As | COMMAND | As | (DATA | As){*len} | P]
  return _write_bytes(smbus_info, command, data, len);
}

esp_err_t smbus_i2c_read_block(const smbus_info_t *smbus_info, uint8_t command,
                               uint8_t *data, size_t len) {
  // Protocol: [S | ADDR | Wr | As | COMMAND | As | Sr | ADDR | Rd | As | (DATAs
  // | A){*len-1} | DATAs | N | P]
  return _read_bytes(smbus_info, command, data, len);
}

static bool _is_tsl2561_init(const tsl2561_info_t *tsl2561_info) {
  bool ok = false;
  if (tsl2561_info != NULL) {
    if (tsl2561_info->init) {
      ok = true;
    } else {
      ESP_LOGE(TAG, "tsl2561_info is not initialised");
    }
  } else {
    ESP_LOGE(TAG, "tsl2561_info is NULL");
  }
  return ok;
}

static bool _check_device_id(tsl2561_device_type_t device) {
  const char *name = NULL;
  switch (device) {
  case TSL2561_DEVICE_TYPE_TSL2560CS:
    name = "0CS";
    break;
  case TSL2561_DEVICE_TYPE_TSL2561CS:
    name = "1CS";
    break;
  case TSL2561_DEVICE_TYPE_TSL2560T_FN_CL:
    name = "0T/FN/CL";
    break;
  case TSL2561_DEVICE_TYPE_TSL2561T_FN_CL:
    name = "1T/FN/CL";
    break;
  default:
    break;
  }
  if (name) {
    ESP_LOGI(TAG, "Device is TSL256%s", name);
  } else {
    ESP_LOGW(TAG, "Device is not recognised");
  }
  return name != NULL;
}

static esp_err_t _power_up(tsl2561_info_t *tsl2561_info) {
  esp_err_t err = ESP_FAIL;
  if (tsl2561_info != NULL) {
    if (!tsl2561_info->powered) {
      if ((err = smbus_write_byte(tsl2561_info->smbus_info,
                                  REG_CONTROL | SMB_COMMAND,
                                  TSL2561_CONTROL_POWER_UP)) == ESP_OK) {
        tsl2561_info->powered = true;
      }
    } else {
      ESP_LOGW(TAG, "Device already powered");
      err = ESP_OK; // not an error
    }
  }
  return err;
}

static esp_err_t _power_down(tsl2561_info_t *tsl2561_info) {
  esp_err_t err = ESP_FAIL;
  if (tsl2561_info != NULL) {
    if (tsl2561_info->powered) {
      if ((err = smbus_write_byte(tsl2561_info->smbus_info,
                                  REG_CONTROL | SMB_COMMAND,
                                  TSL2561_CONTROL_POWER_DOWN)) == ESP_OK) {
        tsl2561_info->powered = false;
      }
    } else {
      ESP_LOGW(TAG, "Device not powered");
      err = ESP_OK; // not an error
    }
  }
  return err;
}

// Assumes device is already powered up
static esp_err_t
_set_integration_time_and_gain(tsl2561_info_t *tsl2561_info,
                               tsl2561_integration_time_t integration_time,
                               tsl2561_gain_t gain) {
  esp_err_t err = ESP_FAIL;
  if (tsl2561_info != NULL && tsl2561_info->powered) {
    if ((err = smbus_write_byte(tsl2561_info->smbus_info,
                                REG_TIMING | SMB_COMMAND,
                                integration_time | gain)) == ESP_OK) {
      tsl2561_info->integration_time = integration_time;
      tsl2561_info->gain = gain;
    }
  }
  return err;
}

// Public API

tsl2561_info_t *tsl2561_malloc(void) {
  tsl2561_info_t *tsl2561_info = malloc(sizeof(*tsl2561_info));
  if (tsl2561_info != NULL) {
    memset(tsl2561_info, 0, sizeof(*tsl2561_info));
    ESP_LOGD(TAG, "malloc tsl2561_info_t %p", tsl2561_info);
  } else {
    ESP_LOGE(TAG, "malloc tsl2561_info_t failed");
  }
  return tsl2561_info;
}

void tsl2561_free(tsl2561_info_t **tsl2561_info) {
  if (tsl2561_info != NULL && (*tsl2561_info != NULL)) {
    ESP_LOGD(TAG, "free tsl2561_info_t %p", *tsl2561_info);
    free(*tsl2561_info);
    *tsl2561_info = NULL;
  } else {
    ESP_LOGE(TAG, "free tsl2561_info_t failed");
  }
}

esp_err_t tsl2561_init(tsl2561_info_t *tsl2561_info, smbus_info_t *smbus_info) {
  esp_err_t err = ESP_FAIL;
  if (tsl2561_info != NULL) {
    tsl2561_info->smbus_info = smbus_info;
    tsl2561_info->powered = false;
    tsl2561_info->integration_time = DEFAULT_INTEGRATION_TIME;
    tsl2561_info->gain = DEFAULT_GAIN;
    tsl2561_info->device_type = TSL2561_DEVICE_TYPE_INVALID;

    tsl2561_info->init = true;

    // read the ID register and confirm that it is as expected for this device
    tsl2561_device_type_t device_type = TSL2561_DEVICE_TYPE_INVALID;
    tsl2561_revision_t revision = 0;
    // for (int retry = 0; retry < 5; retry++)
    // {
    err = tsl2561_device_id(tsl2561_info, &device_type, &revision);
    // ESP_LOGW(TAG, "Device is not recognised... Retrying %d times", retry);
    //     if (err == ESP_OK)
    //         break;
    //     vTaskDelay(1000 / portTICK_PERIOD_MS);
    // }
    if (err == ESP_OK) {
      ESP_LOGI(TAG,
               "Detected device ID 0x%02x, revision %d on I2C address 0x%02x",
               device_type, revision, smbus_info->address);
      if (_check_device_id(device_type)) {
        tsl2561_info->device_type = device_type;
        err = ESP_OK;
      } else {
        ESP_LOGE(TAG, "Unsupported device detected");
      }
    }
  } else {
    ESP_LOGE(TAG, "tsl2561_info is NULL");
    err = ESP_FAIL;
  }
  return err;
}

esp_err_t tsl2561_device_id(const tsl2561_info_t *tsl2561_info,
                            tsl2561_device_type_t *device,
                            tsl2561_revision_t *revision) {
  esp_err_t err = ESP_FAIL;
  if (_is_tsl2561_init(tsl2561_info) && device && revision) {
    uint8_t id = 0;
    err = smbus_read_byte(tsl2561_info->smbus_info, REG_ID | SMB_COMMAND, &id);
    if (err == ESP_OK) {
      *device = (tsl2561_device_type_t)((id >> 4) & 0x0f);
      *revision = (tsl2561_revision_t)(id & 0x0f);
    } else {
      ESP_LOGE(TAG, "Failed to read device ID");
    }
  }
  return err;
}

esp_err_t tsl2561_read(tsl2561_info_t *tsl2561_info, tsl2561_visible_t *visible,
                       tsl2561_infrared_t *infrared) {
  esp_err_t err = ESP_FAIL;
  if (_is_tsl2561_init(tsl2561_info) && visible && infrared) {
    if ((err = _power_up(tsl2561_info)) == ESP_OK) {
      TickType_t delay = 0;
      switch (tsl2561_info->integration_time) {
      case TSL2561_INTEGRATION_TIME_13MS:
        // wait at least 15ms according to Adafruit driver
        delay = 15;
        break;
      case TSL2561_INTEGRATION_TIME_101MS:
        // wait at least 120ms according to Adafruit driver
        delay = 120;
        break;
      default:
        ESP_LOGW(TAG, "Invalid integration time: %d",
                 tsl2561_info->integration_time);
        /* fall through */
      case TSL2561_INTEGRATION_TIME_402MS:
        // wait at least 450ms according to Adafruit driver
        delay = 450;
        break;
      }
      vTaskDelay((delay - 1) / portTICK_PERIOD_MS + 1);

      uint16_t ch0 = 0;
      uint16_t ch1 = 0;
      if ((err = smbus_read_word(tsl2561_info->smbus_info,
                                 REG_DATA0LOW | SMB_COMMAND | SMB_WORD,
                                 &ch0)) == ESP_OK) {
        if ((err = smbus_read_word(tsl2561_info->smbus_info,
                                   REG_DATA1LOW | SMB_COMMAND | SMB_WORD,
                                   &ch1)) == ESP_OK) {
          if ((err = _power_down(tsl2561_info)) == ESP_OK) {
            *visible = ch0 - ch1;
            *infrared = ch1;
          }
        }
      }
    }
  }
  return err;
}

esp_err_t tsl2561_set_integration_time_and_gain(
    tsl2561_info_t *tsl2561_info, tsl2561_integration_time_t integration_time,
    tsl2561_gain_t gain) {
  esp_err_t err = ESP_FAIL;
  if (_is_tsl2561_init(tsl2561_info)) {
    if ((err = _power_up(tsl2561_info)) == ESP_OK) {
      if ((err = _set_integration_time_and_gain(tsl2561_info, integration_time,
                                                gain)) == ESP_OK) {
        tsl2561_info->integration_time = integration_time;
        tsl2561_info->gain = gain;
      }

      esp_err_t pderr = _power_down(tsl2561_info);
      err = pderr != ESP_OK ? pderr : err;
    }
  }
  return err;
}

uint32_t tsl2561_compute_lux(const tsl2561_info_t *tsl2561_info,
                             tsl2561_visible_t visible,
                             tsl2561_infrared_t infrared) {
  uint32_t lux = 0;
  if (_is_tsl2561_init(tsl2561_info)) {
    uint32_t scale = 0;

    // scale channel values
    switch (tsl2561_info->integration_time) {
    case TSL2561_INTEGRATION_TIME_13MS:
      scale = CH_SCALE_TINT0;
      break;
    case TSL2561_INTEGRATION_TIME_101MS:
      scale = CH_SCALE_TINT1;
      break;
    default:
      scale = 1 << CH_SCALE;
    }

    // scale 1x measurement up to 16x
    if (tsl2561_info->gain == TSL2561_GAIN_1X) {
      scale <<= 4;
    }

    // convert visible/infrared back into channel data
    uint32_t channel0 = ((visible + infrared) * scale) >> CH_SCALE;
    uint32_t channel1 = (infrared * scale) >> CH_SCALE;

    // find the ratio of the channel values (channel1/channel0)
    // protect against divide by zero
    uint32_t ratio1 = 0;
    if (channel0 != 0) {
      ratio1 = (channel1 << (RATIO_SCALE + 1)) / channel0;
    }

    // round the ratio value
    uint32_t ratio = (ratio1 + 1) >> 1;

    // is ratio <= eachBreak ?
    int b = 0, m = 0;

    if (ratio <= TSL2561_K1T) {
      b = TSL2561_B1T;
      m = TSL2561_M1T;
    } else if (ratio <= TSL2561_K2T) {
      b = TSL2561_B2T;
      m = TSL2561_M2T;
    } else if (ratio <= TSL2561_K3T) {
      b = TSL2561_B3T;
      m = TSL2561_M3T;
    } else if (ratio <= TSL2561_K4T) {
      b = TSL2561_B4T;
      m = TSL2561_M4T;
    } else if (ratio <= TSL2561_K5T) {
      b = TSL2561_B5T;
      m = TSL2561_M5T;
    } else if (ratio <= TSL2561_K6T) {
      b = TSL2561_B6T;
      m = TSL2561_M6T;
    } else if (ratio <= TSL2561_K7T) {
      b = TSL2561_B7T;
      m = TSL2561_M7T;
    } else if (ratio > TSL2561_K8T) {
      b = TSL2561_B8T;
      m = TSL2561_M8T;
    }
    uint32_t temp = (channel0 * b) - (channel1 * m);

    // prevent negative lux values
    if ((channel1 * m) > (channel0 * b)) {
      temp = 0;
    }

    // round lsb
    temp += (1 << (LUX_SCALE - 1));

    // strip off fractional portion
    lux = temp >> LUX_SCALE;
  }
  return lux;
}

void i2c_master_init(void) {
  int i2c_master_port = I2C_MASTER_NUM;
  i2c_config_t conf;
  conf.mode = I2C_MODE_MASTER;
  conf.sda_io_num = 22;
  conf.sda_pullup_en = GPIO_PULLUP_ENABLE; // GY-2561 provides 10kΩ pullups
  conf.scl_io_num = 23;
  conf.scl_pullup_en = GPIO_PULLUP_ENABLE; // GY-2561 provides 10kΩ pullups
  conf.master.clk_speed = 100000;
  i2c_param_config(i2c_master_port, &conf);
  i2c_driver_install(i2c_master_port, conf.mode, I2C_MASTER_RX_BUF_LEN,
                     I2C_MASTER_TX_BUF_LEN, 0);
}
smbus_info_t *smbus_info = NULL;
tsl2561_info_t *tsl2561_info = NULL;
void initialize_tsl2561(void) {

  i2c_port_t i2c_num = I2C_MASTER_NUM;
  uint8_t address = CONFIG_TSL2561_I2C_ADDRESS;

  // Set up the SMBus
  smbus_info = smbus_malloc();
  smbus_init(smbus_info, i2c_num, address);
  smbus_set_timeout(smbus_info, 1000 / portTICK_PERIOD_MS);

  // Set up the TSL2561 device
  tsl2561_info = tsl2561_malloc();
  tsl2561_init(tsl2561_info, smbus_info);

  // Set sensor integration time and gain
  tsl2561_set_integration_time_and_gain(
      tsl2561_info, TSL2561_INTEGRATION_TIME_402MS, TSL2561_GAIN_1X);
  // tsl2561_set_integration_time_and_gain(tsl2561_info,
  // TSL2561_INTEGRATION_TIME_402MS, TSL2561_GAIN_16X);
}

void get_sensor_values(void) {
  tsl2561_visible_t visible = 0;
  tsl2561_infrared_t infrared = 0;
  tsl2561_read(tsl2561_info, &visible, &infrared);
  ESP_LOGI(TAG, "Full spectrum: %d", visible + infrared);
  ESP_LOGI(TAG, "Infrared:      %d", infrared);
  ESP_LOGI(TAG, "Visible:       %d", visible);
  ESP_LOGI(TAG, "Lux:           %d\n",
           tsl2561_compute_lux(tsl2561_info, visible, infrared));
}