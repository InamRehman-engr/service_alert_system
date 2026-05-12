#include "esp_adc/adc_cali.h"
#include "esp_adc/adc_cali_scheme.h"
#include "esp_adc/adc_continuous.h"
#include "esp_adc/adc_oneshot.h"
#include "esp_check.h"
#include "esp_err.h"
#include "esp_log.h"
#include "hal/adc_types.h"
#include "sdkconfig.h"
#include <stdio.h>
#include <string.h>

typedef struct {
  esp_err_t (*adc_read)(adc_oneshot_unit_handle_t *adc_handle,
                        adc_channel_t channel, int *data);
  adc_cali_handle_t (*adc_calibrate)(adc_unit_t adc_unit, adc_atten_t atten,
                                     adc_bitwidth_t bitwidth,
                                     adc_cali_handle_t *out_handle);
  esp_err_t (*adc_raw_to_voltage)(struct adc_cali_handle_t *handle, int raw,
                                  int *voltage);
  esp_err_t (*adc_de_init)(adc_oneshot_unit_handle_t handle);
} adc_oneshot_functions;

adc_oneshot_functions *adc_init_oneshot(adc_oneshot_unit_handle_t *handle,
                                        adc_unit_t adc_unit,
                                        adc_bitwidth_t bitwidth,
                                        adc_atten_t atten,
                                        adc_channel_t channel);

typedef struct {
  adc_continuous_handle_t handle;
  esp_err_t (*adc_read)(adc_continuous_handle_t handle, uint8_t *data,
                        uint32_t data_length, uint32_t *out_data_length);
  esp_err_t (*adc_stop)(adc_continuous_handle_t handle);
} adc_continuous_functions;

adc_continuous_functions
adc_init_continuous(adc_channel_t *channel, adc_digi_convert_mode_t conv_mode,
                    adc_digi_output_format_t output_format, int num_of_channels,
                    int buf_size, int read_len, uint32_t sampling_freq,
                    int bit_width, int adc_unit, int adc_atten);
