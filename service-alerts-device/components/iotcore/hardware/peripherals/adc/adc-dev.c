#include "adc-dev.h"

esp_err_t adc_read_oneshot(adc_oneshot_unit_handle_t *adc_handle,
                           adc_channel_t channel, int *data) {
  esp_err_t ret = adc_oneshot_read(*adc_handle, channel, data);
  return ret;
}
esp_err_t adc_raw_to_voltage(adc_cali_handle_t handle, int raw, int *voltage) {
  return adc_cali_raw_to_voltage(handle, raw, voltage);
}

esp_err_t adc_de_init(adc_oneshot_unit_handle_t handle) {
  return (adc_oneshot_del_unit(handle));
}
adc_cali_handle_t adc_calibrate_oneshot(adc_unit_t adc_unit, adc_atten_t atten,
                                        adc_bitwidth_t bitwidth,
                                        adc_cali_handle_t *out_handle) {
  adc_cali_handle_t handle = NULL;
  bool calibrated = false;
#if ADC_CALI_SCHEME_CURVE_FITTING_SUPPORTED
  if (!calibrated) {
    ESP_LOGI("ADC-Oneshot", "calibration scheme version is %s",
             "Curve Fitting");
    adc_cali_curve_fitting_config_t cali_config = {
        .unit_id = adc_unit,
        .atten = atten,
        .bitwidth = bitwidth,
    };
    ret = adc_cali_create_scheme_curve_fitting(&cali_config, &handle);
    if (ret == ESP_OK) {
      calibrated = true;
    }
  }
#endif

#if ADC_CALI_SCHEME_LINE_FITTING_SUPPORTED
  esp_err_t ret;
  if (!calibrated) {
    ESP_LOGI("ADC-Oneshot", "calibration scheme version is %s", "Line Fitting");
    adc_cali_line_fitting_config_t cali_config = {
        .unit_id = adc_unit,
        .atten = atten,
        .bitwidth = bitwidth,
    };
    ret = adc_cali_create_scheme_line_fitting(&cali_config, &handle);
    if (ret == ESP_OK) {
      calibrated = true;
    }
  }
#endif

  out_handle = handle;
  if (out_handle == NULL) {
    printf(" Out Handle is NULL");
  }
  if (ret == ESP_OK) {
    ESP_LOGI("ADC-Oneshot", "Calibration Success");
  } else if (ret == ESP_ERR_NOT_SUPPORTED || !calibrated) {
    ESP_LOGW("ADC-Oneshot", "eFuse not burnt, skip software calibration");
  } else {
    ESP_LOGE("ADC-Oneshot", "Invalid arg or no memory");
  }
  return handle;
}

adc_oneshot_functions *adc_init_oneshot(adc_oneshot_unit_handle_t *handle,
                                        adc_unit_t adc_unit,
                                        adc_bitwidth_t bitwidth,
                                        adc_atten_t atten,
                                        adc_channel_t channel) {
  adc_oneshot_functions *functions_configs =
      malloc(sizeof(adc_oneshot_functions));
  functions_configs->adc_read = adc_read_oneshot;
  functions_configs->adc_calibrate = adc_calibrate_oneshot;
  functions_configs->adc_raw_to_voltage = adc_raw_to_voltage;
  functions_configs->adc_de_init = adc_de_init;
  adc_oneshot_unit_init_cfg_t init_config1 = {
      .unit_id = 0,
  };

  esp_err_t err = adc_oneshot_new_unit(&init_config1, handle);
  printf("The Error is %d\n", err);
  const adc_oneshot_chan_cfg_t config = {
      .bitwidth = bitwidth,
      .atten = atten,
  };
  ESP_LOGE("ADC", "Channel = %d", channel);
  adc_oneshot_config_channel(*handle, channel, &config);
  return functions_configs;
}

esp_err_t adc_read_continuous(adc_continuous_handle_t handle, uint8_t *data,
                              uint32_t data_length, uint32_t *out_data_length) {
  esp_err_t ret =
      adc_continuous_read(handle, data, data_length, out_data_length, 0);
  return ret;
}
esp_err_t adc_stop_continuous(adc_continuous_handle_t handle) {
  return (adc_continuous_stop(handle));
}
adc_continuous_functions
adc_init_continuous(adc_channel_t *channel, adc_digi_convert_mode_t conv_mode,
                    adc_digi_output_format_t output_format, int num_of_channels,
                    int buf_size, int read_len, uint32_t sampling_freq,
                    int bit_width, int adc_unit, int adc_atten) {
  adc_continuous_functions functions_configs = {
      .handle = NULL,
  };

  adc_continuous_handle_cfg_t adc_config = {
      .max_store_buf_size = buf_size,
      .conv_frame_size = read_len,
  };
  adc_continuous_new_handle(&adc_config, &functions_configs.handle);

  adc_continuous_config_t dig_cfg = {
      .sample_freq_hz = sampling_freq,
      .conv_mode = conv_mode,
      .format = output_format,
  };

  adc_digi_pattern_config_t adc_pattern[SOC_ADC_PATT_LEN_MAX] = {0};
  dig_cfg.pattern_num = num_of_channels;
  for (int i = 0; i < num_of_channels; i++) {
    uint8_t ch = channel[i] & 0x7;
    adc_pattern[i].atten = adc_atten;
    adc_pattern[i].channel = ch;
    adc_pattern[i].unit = adc_unit;
    adc_pattern[i].bit_width = bit_width;
  }
  dig_cfg.adc_pattern = adc_pattern;
  adc_continuous_config(functions_configs.handle, &dig_cfg);
  // adc_continuous_evt_cbs_t cbs = {
  //     .on_conv_done = callBack_ptr,
  // };
  // adc_continuous_register_event_callbacks(functions_configs.handle, &cbs,
  // NULL);
  adc_continuous_start(functions_configs.handle);
  return functions_configs;
  // *out_handle = functions_configs.handle;
}