#include "MC106.h"

// ADC Channels
// ADC Attenuation
#define ADC_EXAMPLE_ATTEN ADC_ATTEN_DB_12

// ADC Calibration
#define ADC_EXAMPLE_CALI_SCHEME ESP_ADC_CAL_VAL_EFUSE_VREF
esp_err_t ret = ESP_OK;
esp_err_t ret2 = ESP_OK;
bool cali_enable = false;
static int adc_raw[2][10];
static const uint sample = 150;
int value1, value2, val_diff, final;
static const char *TAG = "ADC Output";
uint32_t voltage1 = 0;
uint32_t voltage2 = 0;

static esp_adc_cal_characteristics_t adc_chars;

void adc_calibration_init(void) {
  ret = esp_adc_cal_check_efuse(ADC_EXAMPLE_CALI_SCHEME);
  if (ret == ESP_ERR_NOT_SUPPORTED) {
    ESP_LOGW(TAG,
             "Calibration scheme not supported, skip software calibration");
  } else if (ret == ESP_ERR_INVALID_VERSION) {
    ESP_LOGW(TAG, "eFuse not burnt, skip software calibration");
  } else if (ret == ESP_OK) {
    ESP_LOGI(TAG, "Calibration scheme supported, Software calibration.....");
    cali_enable = true;
    esp_adc_cal_characterize(ADC_UNIT_1, ADC_EXAMPLE_ATTEN,
                             ADC_WIDTH_BIT_DEFAULT, 0, &adc_chars);
    esp_adc_cal_characterize(ADC_UNIT_2, ADC_EXAMPLE_ATTEN,
                             ADC_WIDTH_BIT_DEFAULT, 0, &adc_chars);
  } else {
    ESP_LOGE(TAG, "Invalid arg");
  }
}
esp_err_t ADC_init(void) {

  adc_calibration_init();
  if (CONFIG_MC106_ADC_UNIT == 1)
  // ADC1 config
  {
    ESP_ERROR_CHECK(adc1_config_width(ADC_WIDTH_BIT_DEFAULT));
    ESP_ERROR_CHECK(adc1_config_channel_atten(CONFIG_MC106_ADC_CHANNEL_A,
                                              ADC_EXAMPLE_ATTEN));
    ESP_ERROR_CHECK(adc1_config_channel_atten(CONFIG_MC106_ADC_CHANNEL_B,
                                              ADC_EXAMPLE_ATTEN));
  } else {
    // ADC2 config
    ESP_ERROR_CHECK(adc2_config_channel_atten(CONFIG_MC106_ADC_CHANNEL_A,
                                              ADC_EXAMPLE_ATTEN));
    ESP_ERROR_CHECK(adc2_config_channel_atten(CONFIG_MC106_ADC_CHANNEL_B,
                                              ADC_EXAMPLE_ATTEN));
  }
  return ESP_OK;
}
void get_MC106(void) {

  uint avg = 0;
  for (int a = 0; a < sample; a++) {
    if (CONFIG_MC106_ADC_UNIT == 1) {
      adc_raw[0][0] = adc1_get_raw(CONFIG_MC106_ADC_CHANNEL_A);
      adc_raw[1][0] = adc1_get_raw(CONFIG_MC106_ADC_CHANNEL_B);
    } else {
      do {
        ret = adc2_get_raw(CONFIG_MC106_ADC_CHANNEL_A, ADC_WIDTH_BIT_DEFAULT,
                           &adc_raw[0][0]);
        ret2 = adc2_get_raw(CONFIG_MC106_ADC_CHANNEL_B, ADC_WIDTH_BIT_DEFAULT,
                            &adc_raw[1][0]);
      } while (ret == ESP_ERR_INVALID_STATE);
      ESP_ERROR_CHECK(ret && ret2);
    }

    if (cali_enable) {
      voltage1 = esp_adc_cal_raw_to_voltage(adc_raw[0][0], &adc_chars);
      voltage2 = esp_adc_cal_raw_to_voltage(adc_raw[1][0], &adc_chars);
    }

    val_diff = voltage1 - voltage2;
    val_diff = abs(val_diff);
    avg += val_diff;
  }
  final = avg / sample;
  if (final > 100)
    ESP_LOGW(TAG, "Final data: %d mV", final);
  else
    ESP_LOGI(TAG, "Final data: %d mV", final);
}
