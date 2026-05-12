#include "OPT3001.h"

static const char *TAG = "OPT300x";

#define WRITE_BIT I2C_MASTER_WRITE
#define READ_BIT I2C_MASTER_READ
#define ACK_CHECK true
#define NO_ACK_CHECK false
#define ACK_VALUE 0x0
#define NACK_VALUE 0x1
#define MAX_BLOCK_LEN 255 // SMBus v3.0 increases this from 32 to 255
// #define MEASURE               // enable measurement and reporting of I2C
// transaction duration

#define I2C_MASTER_NUM I2C_NUM_0
#define I2C_MASTER_TX_BUF_LEN 0 // disabled
#define I2C_MASTER_RX_BUF_LEN 0 // disabled
#define I2C_MASTER_FREQ_HZ 100000
#define I2C_MASTER_SDA_IO CONFIG_I2C_MASTER_SDA
#define I2C_MASTER_SCL_IO CONFIG_I2C_MASTER_SCL

// Register addresses
#define REG_RESULT 0x00
#define REG_CONFIGURATION 0x01
#define REG_LOWLIMIT 0x02
#define REG_HIGHLIMIT 0x03
#define REG_MANUFACTURERID 0x7E
#define REG_DEVICEID 0x7F

// Register Bits
// REG_CONFIGURATION
#define RANGE 0XF000
#define CT 0X0800
#define MODE 0X0300
#define OVF 0X0100
#define CRF 0X0080
#define FH 0X0040
#define FL 0X0020
#define LATCH 0X0010
#define POL 0X0008
#define ME 0X0004
#define FC 0X0003

// Limit Set Bits
#define EXPONENTIAL 0XF000
#define THRESHOLD 0X0FFF

// Modes
#define SHUTDOWN 0x00
#define SINGLESHOT 0X01
#define CONTINOUS 0X02

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

esp_err_t smbus_write_word(const smbus_info_t *smbus_info, uint8_t command,
                           uint16_t data) {
  // Protocol: [S | ADDR | Wr | As | COMMAND | As | DATA-HIGH | As | DATA-LOW |
  // As | P]
  uint8_t temp[2] = {(data >> 8) & 0xff, data & 0xff};
  return _write_bytes(smbus_info, command, temp, 2);
}

esp_err_t smbus_read_word(const smbus_info_t *smbus_info, uint8_t command,
                          uint16_t *data) {
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

static bool _is_opt3001_init(const opt3001_info_t *opt3001_info) {
  bool ok = false;
  if (opt3001_info != NULL) {
    if (opt3001_info->init) {
      ok = true;
    } else {
      ESP_LOGE(TAG, "opt3001_info is not initialised");
    }
  } else {
    ESP_LOGE(TAG, "opt3001_info is NULL");
  }
  return ok;
}

// Public API

opt3001_info_t *opt3001_malloc(void) {
  opt3001_info_t *opt3001_info = malloc(sizeof(*opt3001_info));
  if (opt3001_info != NULL) {
    memset(opt3001_info, 0, sizeof(*opt3001_info));
    ESP_LOGD(TAG, "malloc opt3001_info_t %p", opt3001_info);
  } else {
    ESP_LOGE(TAG, "malloc opt3001_info_t failed");
  }
  return opt3001_info;
}

void opt3001_free(opt3001_info_t **opt3001_info) {
  if (opt3001_info != NULL && (*opt3001_info != NULL)) {
    ESP_LOGD(TAG, "free opt3001_info_t %p", *opt3001_info);
    free(*opt3001_info);
    *opt3001_info = NULL;
  } else {
    ESP_LOGE(TAG, "free opt3001_info_t failed");
  }
}

esp_err_t opt3001_init(opt3001_info_t *opt3001_info, smbus_info_t *smbus_info) {
  esp_err_t err = ESP_FAIL;
  if (opt3001_info != NULL) {
    opt3001_info->smbus_info = smbus_info;

    opt3001_info->init = true;

    // read the ID register and confirm that it is as expected for this device
    device_type_t device_type = 0;
    err = opt3001_device_id(opt3001_info, &device_type);
    if (err == ESP_OK) {
      ESP_LOGI(TAG, "Detected device ID 0x%02x on I2C address 0x%02x",
               device_type, smbus_info->address);
      opt3001_info->device_type = device_type;
      err = ESP_OK;
    } else {
      ESP_LOGE(TAG, "Unsupported device detected");
    }
  } else {
    ESP_LOGE(TAG, "opt3001_info is NULL");
    err = ESP_FAIL;
  }
  return err;
}

esp_err_t opt3001_device_id(const opt3001_info_t *opt3001_info,
                            device_type_t *device) {
  esp_err_t err = ESP_FAIL;
  if (_is_opt3001_init(opt3001_info) && device) {
    err = smbus_read_word(opt3001_info->smbus_info, REG_DEVICEID, device);
    if (err == ESP_OK) {
      ESP_LOGI(TAG, "Read device ID Successfully");
    } else {
      ESP_LOGE(TAG, "Failed to read device ID");
    }
  }
  return err;
}

esp_err_t opt3001_read(opt3001_info_t *opt3001_info,
                       data_register_t *lower_byte,
                       data_register_t *upper_byte) {
  esp_err_t err = ESP_FAIL;
  if (_is_opt3001_init(opt3001_info) && lower_byte && upper_byte) {

    TickType_t delay = 0;
    data_register_t current_configuration = 0;
    smbus_read_word(opt3001_info->smbus_info, REG_CONFIGURATION,
                    &current_configuration);
    err = smbus_write_word(opt3001_info->smbus_info, REG_CONFIGURATION,
                           (1 << 11) | current_configuration);
    if (err == ESP_OK) {
      delay = 800;
    } else {
      delay = 15;
    }
    vTaskDelay((delay - 1) / portTICK_PERIOD_MS + 1);

    data_register_t temp = 0;
    if ((err = smbus_read_word(opt3001_info->smbus_info, REG_RESULT, &temp)) ==
        ESP_OK) {
      *lower_byte = temp & 0XFF;
      *upper_byte = ((temp & 0XFF00) >> 8);
    }
  }
  return err;
}

// uint32_t opt3001_compute_lux(const opt3001_info_t *opt3001_info,
// data_register_t *lower_byte, data_register_t *upper_byte)
// {
//     uint32_t lux = 0;
// if (_is_opt3001_init(opt3001_info))
// {
//     uint32_t scale = 0;

//     // scale channel values
//     switch (opt3001_info->integration_time)
//     {
//     case TSL2561_INTEGRATION_TIME_13MS:
//         scale = CH_SCALE_TINT0;
//         break;
//     case TSL2561_INTEGRATION_TIME_101MS:
//         scale = CH_SCALE_TINT1;
//         break;
//     default:
//         scale = 1 << CH_SCALE;
//     }

//     // convert lower_byte/upper_byte back into channel data
//     uint32_t channel0 = ((lower_byte + upper_byte) * scale) >> CH_SCALE;
//     uint32_t channel1 = (upper_byte * scale) >> CH_SCALE;

//     // find the ratio of the channel values (channel1/channel0)
//     // protect against divide by zero
//     uint32_t ratio1 = 0;
//     if (channel0 != 0)
//     {
//         ratio1 = (channel1 << (RATIO_SCALE + 1)) / channel0;
//     }

//     // round the ratio value
//     uint32_t ratio = (ratio1 + 1) >> 1;

//     // is ratio <= eachBreak ?
//     int b = 0, m = 0;

//     if (ratio <= TSL2561_K1T)
//     {
//         b = TSL2561_B1T;
//         m = TSL2561_M1T;
//     }
//     else if (ratio <= TSL2561_K2T)
//     {
//         b = TSL2561_B2T;
//         m = TSL2561_M2T;
//     }
//     else if (ratio <= TSL2561_K3T)
//     {
//         b = TSL2561_B3T;
//         m = TSL2561_M3T;
//     }
//     else if (ratio <= TSL2561_K4T)
//     {
//         b = TSL2561_B4T;
//         m = TSL2561_M4T;
//     }
//     else if (ratio <= TSL2561_K5T)
//     {
//         b = TSL2561_B5T;
//         m = TSL2561_M5T;
//     }
//     else if (ratio <= TSL2561_K6T)
//     {
//         b = TSL2561_B6T;
//         m = TSL2561_M6T;
//     }
//     else if (ratio <= TSL2561_K7T)
//     {
//         b = TSL2561_B7T;
//         m = TSL2561_M7T;
//     }
//     else if (ratio > TSL2561_K8T)
//     {
//         b = TSL2561_B8T;
//         m = TSL2561_M8T;
//     }
//     uint32_t temp = (channel0 * b) - (channel1 * m);

//     // prevent negative lux values
//     if ((channel1 * m) > (channel0 * b))
//     {
//         temp = 0;
//     }

//     // round lsb
//     temp += (1 << (LUX_SCALE - 1));

//     // strip off fractional portion
//     lux = temp >> LUX_SCALE;
// }
//     return lux;
// }

void OPT3001_i2c_master_init(void) {
  int i2c_master_port = I2C_MASTER_NUM;
  i2c_config_t conf;
  conf.mode = I2C_MODE_MASTER;
  conf.sda_io_num = CONFIG_I2C_MASTER_SDA;
  conf.sda_pullup_en = GPIO_PULLUP_ENABLE; // GY-2561 provides 10kΩ pullups
  conf.scl_io_num = CONFIG_I2C_MASTER_SCL;
  conf.scl_pullup_en = GPIO_PULLUP_ENABLE; // GY-2561 provides 10kΩ pullups
  conf.master.clk_speed = 100000;

  i2c_param_config(i2c_master_port, &conf);
  i2c_driver_install(i2c_master_port, conf.mode, I2C_MASTER_RX_BUF_LEN,
                     I2C_MASTER_TX_BUF_LEN, 0);
}

smbus_info_t *smbus_info = NULL;
opt3001_info_t *opt3001_info = NULL;

void initialize_opt3001(void) {

  i2c_port_t i2c_num = I2C_MASTER_NUM;
  uint8_t address = CONFIG_OPT3001_I2C_ADDRESS;

  // Set up the SMBus
  smbus_info = smbus_malloc();
  smbus_init(smbus_info, i2c_num, address);
  smbus_set_timeout(smbus_info, 1000 / portTICK_PERIOD_MS);

  // Set up the OPT3001 device
  opt3001_info = opt3001_malloc();

  opt3001_init(opt3001_info, smbus_info);
}

void get_sensor_values(void) {
  data_register_t lower_byte = 0;
  data_register_t upper_byte = 0;
  opt3001_read(opt3001_info, &lower_byte, &upper_byte);
  uint32_t lux_value =
      opt3001_compute_lux(opt3001_info, &lower_byte, &upper_byte);
  ESP_LOGI(TAG, "upper_byte:      %d", upper_byte);
  ESP_LOGI(TAG, "lower_byte:      %d", lower_byte);
  // ESP_LOGI(TAG, "Lux:             %d\n",lux_value);
}