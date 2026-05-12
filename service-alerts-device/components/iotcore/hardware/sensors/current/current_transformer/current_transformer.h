#ifndef _CT_h_
#define _CT_h_
#include "esp_adc/adc_continuous.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/task.h"
extern uint32_t SAMPLING_FREQUENCY;

#define FREQUENCY_OUT_OF_RANGE 290
#define ADC_CHANNELS_OUT_OF_RANGE 291

// CT Types
typedef enum { CT_30A_1V, CT_100A_1V, CT_50A_1V, CT_100A_50mV } CT;

typedef struct {
  QueueHandle_t data_queue_handle;
  CT type;
  adc_channel_t adc_channel;
} CT_data;

typedef struct {
  uint32_t sampling_frequency;
  uint8_t no_of_adc_channels;
  CT_data *cts;
} CT_config_t;

extern TaskHandle_t CT_Task_Handle;

void CT_init(CT_config_t *configs);
esp_err_t validate_config(CT_config_t *configs);
void ADC_read(esp_err_t ret, uint32_t ret_num, uint8_t *result,
              double *average_outer, int *count);
void ADC_average_out(adc_continuous_handle_t *handle);

#endif