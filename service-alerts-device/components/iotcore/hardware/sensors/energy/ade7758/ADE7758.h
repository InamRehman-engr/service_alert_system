#ifndef _ADE7758_H_
#define _ADE7758_H_

#include "esp_err.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "math.h"
#include <spi_master_dev.h>

#define AWATTHR 0x01
#define BWATTHR 0x02
#define CWATTHR 0x03
#define AVARHR 0x04
#define BVARHR 0x05
#define CVARHR 0x06
#define AVAHR 0x07
#define BVAHR 0x08
#define CVAHR 0x09
#define AIRMS 0x0A
#define BIRMS 0x0B
#define CIRMS 0x0C
#define AVRMS 0x0D
#define BVRMS 0x0E
#define CVRMS 0x0F
#define FREQ 0x10
#define TEMP 0x11
#define WFORM 0x12
#define OPMODE 0x13
#define MMODE 0x14
#define WAVMODE 0x15
#define COMPMODE 0x16
#define LCYCMODE 0x17
#define MASK 0x18
#define STATUS 0x19
#define RSTATUS 0x1A
#define ZXTOUT 0x1B
#define LINECYC 0x1C
#define SAGCYC 0x1D
#define SAGLVL 0x1E
#define VPINTLVL 0x1F
#define IPINTLVL 0x20
#define VPEAK 0x21
#define IPEAK 0x22
#define GAIN 0x23
#define AVRMSGAIN 0x24
#define BVRMSGAIN 0x25
#define CVRMSGAIN 0x26
#define AIGAIN 0x27
#define BIGAIN 0x28
#define CIGAIN 0x29
#define AWG 0x2A
#define BWG 0x2B
#define CWG 0x2C
#define AVARG 0x2D
#define BVARG 0x2E
#define CVARG 0x2F
#define AVAG 0x30
#define BVAG 0x31
#define CVAG 0x32
#define AVRMSOS 0x33
#define BVRMSOS 0x34
#define CVRMSOS 0x35
#define AIRMSOS 0x36
#define BIRMSOS 0x37
#define CIRMSOS 0x38
#define AWAITOS 0x39
#define BWAITOS 0x3A
#define CWAITOS 0x3B
#define AVAROS 0x3C
#define BVAROS 0x3D
#define CVAROS 0x3E
#define APHCAL 0x3F
#define BPHCAL 0x40
#define CPHCAL 0x41
#define WDIV 0x42
#define VADIV 0x44
#define VARDIV 0x43
#define APCFNUM 0x45
#define APCFDEN 0x46
#define VARCFNUM 0x47
#define VARCFDEN 0x48
#define CHKSUM 0x7E
#define VERSION 0x7F

#define SPI_REG_READ(reg) reg
#define SPI_REG_WRITE(reg) (uint8_t)((reg) | 0x80)

// PHASE_SEL
#define PHASE_A 0
#define PHASE_B 1
#define PHASE_C 2
#define ALL_PHASE 3

// WAV_SEL
#define CURRENT 0
#define VOLTAGE 1
#define ACT_PWR 2
#define REACT_PWR 3
#define APP_PWR 4

// interrupt mask/status bit
#define AEHF 0
#define REHF 1
#define VAEHF 2
#define SAGA 3
#define SAGB 4
#define SAGC 5
#define ZXTOA 6
#define ZXTOB 7
#define ZXTOC 8
#define ZXA 9
#define ZXB 10
#define ZXC 11
#define LENERGY 12
#define RESET 13
#define PKV 14
#define PKI 15
#define WFSM 16
#define REVPAP 17
#define REVPRP 18
#define SEQERR 19

#define WAVMODE_VALUE(phase, wave) (((wave) << 2) | (phase))

#define SET_BIT(reg, pos) ((reg) |= (1U << (pos)))
#define CLEAR_BIT(reg, pos) ((reg) &= ~(1U << (pos)))
#define TOGGLE_BIT(reg, pos) ((reg) ^= (1U << (pos)))

#define CONVERT_12_TO_16(value12bit)                                           \
  ((int16_t)(((value12bit) & 0x0800) ? (0xF000 | (value12bit)) : (value12bit)))

#define fmap(x, in_min, in_max, out_min, out_max)                              \
  (((x) - (in_min)) * ((out_max) - (out_min)) / ((in_max) - (in_min)) +        \
   (out_min))

#define calculate_Voffset(Vnom, Vmin, Vrmsmin, Vrmsnom)                        \
  round(((1.0f / 64.0f) * ((Vnom * Vrmsmin) - (Vmin * Vrmsnom))) /             \
        (Vmin - Vnom));

#define calculate_Ioffset(Itest, Imin, Irmsmin, Irmstest)                      \
  round(((1.0f / 16384.0f) * ((pow(Itest, 2) * pow(Irmsmin, 2)) -              \
                              (pow(Imin, 2) * pow(Irmstest, 2)))) /            \
        (pow(Imin, 2) - pow(Itest, 2)));

#define HANDLE_RESPONSE_CALL(configs, args)                                    \
  {                                                                            \
    if (configs->calibration_response_cb != NULL) {                            \
      configs->calibration_response_cb(args);                                  \
    } else {                                                                   \
      ESP_LOGE(TAG, "Please initialize response callback");                    \
    }                                                                          \
  }

#define SET_RESPONSE_TYPE(data_type, res, msg)                                 \
  {                                                                            \
    res->type = data_type;                                                     \
    res->phase = msg.phase;                                                    \
    res->status = true;                                                        \
  }

enum data_type {
  CALIBRATE_ITEST,
  CALIBRATE_IMIN,
  CALIBRATE_VNOM,
  CALIBRATE_VMIN,
  CALIBRATE_VSCALE_FACTOR,
  CALIBRATE_ISCALE_FACTOR,
  RESPONSE_ITEST,
  RESPONSE_IMIN,
  RESPONSE_VNOM,
  RESPONSE_VMIN,
  RESPONSE_VSCALE_FACTOR,
  RESPONSE_ISCALE_FACTOR,
  RESPONSE_IOFFSET,
  RESPONSE_VOFFSET,
  RESPONSE_CAL_TASK_STARTED,
};

typedef struct {
  enum data_type type;
  uint8_t phase;
  uint8_t samples;
  union {
    float vmin;
    float vnom;
    float itest;
    float imin;
    double v_factor;
    double i_factor;
  };
} ade7758_calibration_comands_t;

typedef struct {
  enum data_type type;
  uint8_t phase;
  bool status;
  union {
    double vrmsmin[3];
    double vrmsnom[3];
    double irmstest[3];
    double irmsmin[3];
    int16_t ioffset[3];
    int16_t Voffset[3];
    double I_cal_factor[3];
    double V_cal_factor[3];
  };
} ade7758_calibration_response_t;

typedef struct {
  double current_factor[3];
  double voltage_factor[3];
} ade7758_calibration_t;

typedef enum {
  NORMAL_OPERATION,
  SWITCH_OFF_CURRENT_ADCS,
  SWITCH_OFF_VOLTAGE_ADCS,
  SLEEP_MODE,
  REDIRECT_VOLTAGE_TO_CURRENT_PATHS,
  REDIRECT_CURRENT_ADC_TO_VOLTAGE,
  REDIRECT_VOLTAGE_ADC_TO_CURRENT,
  POWER_DOWN_MODE
} ade7758_opmode;

typedef struct ade7758_handle_t ade7758_handle_t;

struct ade7758_handle_t {
  spi_master_functions spi;
  gpio_num_t IRQ;
  struct ade7758_data_t {
    float temperature;
    uint16_t frequency;
    float current[3];
    float voltages[3];
    float Reactive_Power[3];
    float Apparent_Power[3];
    float energy[3];
  } data;
  ade7758_calibration_t calibration_data; // Pass these from outside
  uint64_t update_interval_ms;
  uint8_t power_save_mode; // 0 = None, 1 = Enable
  bool is_calibration_running;
  esp_err_t (*start_calibration)(ade7758_handle_t *handle);
  bool (*send_calibration_message)(ade7758_calibration_comands_t *commands);
  void (*calibration_response_cb)(ade7758_calibration_response_t *response);
};

/**
 * @brief This will start the reading
 *
 * @param handle
 */
void ade7758_init(ade7758_handle_t *handle);

#endif