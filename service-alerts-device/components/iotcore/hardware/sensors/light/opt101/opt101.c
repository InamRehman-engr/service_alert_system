#include "opt101.h"

#define TAG "opt101"
#define DEFAULT_VREF 2450
#define NO_OF_SAMPLES 150 // Multisampling

static const adc_channel_t channel = CONFIG_OPT101_ADC_CHANNEL;
static const adc_bits_width_t width = ADC_WIDTH_BIT_12;

static const adc_atten_t atten = ADC_ATTEN_DB_12;
static const adc_unit_t unit = CONFIG_OPT101_ADC_UNIT;

static esp_adc_cal_characteristics_t *adc_chars = NULL;

static void check_efuse(void) {
  // Check if TP is burned into eFuse
  if (esp_adc_cal_check_efuse(ESP_ADC_CAL_VAL_EFUSE_TP) == ESP_OK) {
    ESP_LOGI(TAG, "eFuse Two Point: Supported");
  } else {
    ESP_LOGW(TAG, "eFuse Two Point: NOT supported");
  }
  // Check Vref is burned into eFuse
  if (esp_adc_cal_check_efuse(ESP_ADC_CAL_VAL_EFUSE_VREF) == ESP_OK) {
    ESP_LOGI(TAG, "eFuse Vref: Supported");
  } else {
    ESP_LOGW(TAG, "eFuse Vref: NOT supported");
  }
}

static void print_char_val_type(esp_adc_cal_value_t val_type) {
  if (val_type == ESP_ADC_CAL_VAL_EFUSE_TP) {
    ESP_LOGI(TAG, "Characterized using Two Point Value");
  } else if (val_type == ESP_ADC_CAL_VAL_EFUSE_VREF) {
    ESP_LOGI(TAG, "Characterized using eFuse Vref");
  } else {
    ESP_LOGI(TAG, "Characterized using Default Vref");
  }
}

esp_err_t adc_init(void) {
  // Check if Two Point or Vref are burned into eFuse
  check_efuse();

  // Configure ADC
  if (unit == ADC_UNIT_1) {
    adc1_config_width(width);
    adc1_config_channel_atten(channel, atten);
  } else {
    adc2_config_channel_atten((adc2_channel_t)channel, atten);
  }

  // Characterize ADC
  adc_chars = calloc(1, sizeof(esp_adc_cal_characteristics_t));
  esp_adc_cal_value_t val_type =
      esp_adc_cal_characterize(unit, atten, width, DEFAULT_VREF, adc_chars);
  print_char_val_type(val_type);
  return ESP_OK;
}

uint32_t get_adc() {
  if (adc_chars == NULL) {
    ESP_LOGE(TAG, "ADC Not Initialized");
    return 0;
  }
  adc_power_on();
  uint32_t adc_reading = 0;
  // Multisampling
  for (int i = 0; i < NO_OF_SAMPLES; i++) {
    if (unit == ADC_UNIT_1) {
      adc_reading += adc1_get_raw((adc1_channel_t)channel);
    } else {
      int raw;
      esp_err_t esp_err =
          adc2_get_raw((adc2_channel_t)channel, ADC_WIDTH_BIT_12, &raw);
      if (esp_err == ESP_OK) {
        // printf("raw : %d\n", raw);
        adc_reading += raw;
      } else if (esp_err == ESP_ERR_TIMEOUT) {
        ESP_LOGE(TAG, "ADC2 Timeout");
      } else {
        ESP_LOGE(TAG, "%s", esp_err_to_name(esp_err));
      }
    }
  }
  adc_reading /= NO_OF_SAMPLES;
  // Convert adc_reading to voltage in mV
  uint32_t voltage = esp_adc_cal_raw_to_voltage(adc_reading, adc_chars);
  ESP_LOGI(TAG, "Raw: %d\tVoltage: %dmV", adc_reading, voltage);
  adc_power_off();
  return voltage;
}
