#include "current_transformer.h"
#include "esp_log.h"
#include <math.h>
#include <string.h>

#if CONFIG_IDF_TARGET_ESP32 || CONFIG_IDF_TARGET_ESP32S2
#define SOC_ADC_OUTPUT_TYPE ADC_DIGI_OUTPUT_FORMAT_TYPE1
#define SOC_ADC_GET_CHANNEL(p_data) ((p_data)->type1.channel)
#define SOC_ADC_GET_DATA(p_data) ((p_data)->type1.data)
#else
#define SOC_ADC_OUTPUT_TYPE ADC_DIGI_OUTPUT_FORMAT_TYPE2
#define SOC_ADC_GET_CHANNEL(p_data) ((p_data)->type2.channel)
#define SOC_ADC_GET_DATA(p_data) ((p_data)->type2.data)
#endif

#define no_of_scans 250
static const char *TAG = "CT";

TaskHandle_t CT_Task_Handle;

static adc_channel_t list_of_adc_channels[8];
int no_of_channels;
QueueHandle_t calculator_queue_handle;
QueueHandle_t *current_queue[8];
CT ct_to_encode[8];

float fmap(float x, float in_min, float in_max, float out_min, float out_max) {
  return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}

static bool check_valid_data(const adc_digi_output_data_t *data) {
  if (SOC_ADC_GET_CHANNEL(data) >= SOC_ADC_CHANNEL_NUM(ADC_UNIT_1)) {
    return false;
  }
  return true;
}

static bool IRAM_ATTR s_conv_done_cb(adc_continuous_handle_t handle,
                                     const adc_continuous_evt_data_t *edata,
                                     void *user_data) {
  BaseType_t mustYield = pdFALSE;
  // Notify that ADC continuous driver has done enough number of conversions
  vTaskNotifyGiveFromISR(CT_Task_Handle, &mustYield);

  return (mustYield == pdTRUE);
}

static void continuous_adc_init(adc_channel_t *channel, uint8_t channel_num,
                                adc_continuous_handle_t *out_handle,
                                uint32_t read_len,
                                uint32_t sampling_frequency) {
  adc_continuous_handle_t handle = NULL;

  adc_continuous_handle_cfg_t adc_config = {
      .max_store_buf_size = read_len,
      .conv_frame_size = read_len,
  };
  ESP_ERROR_CHECK(adc_continuous_new_handle(&adc_config, &handle));

  adc_continuous_config_t dig_cfg = {
      .sample_freq_hz = sampling_frequency,
      .conv_mode = ADC_CONV_SINGLE_UNIT_1,
      .format = SOC_ADC_OUTPUT_TYPE,
  };

  adc_digi_pattern_config_t adc_pattern[SOC_ADC_PATT_LEN_MAX] = {0};
  dig_cfg.pattern_num = channel_num;

  for (int i = 0; i < channel_num; i++) {
    uint8_t unit = ADC_UNIT_1;
    uint8_t ch = channel[i] & 0x7;
    adc_pattern[i].atten = ADC_ATTEN_DB_12;
    adc_pattern[i].channel = ch;
    adc_pattern[i].unit = unit;
    adc_pattern[i].bit_width = SOC_ADC_DIGI_MAX_BITWIDTH;

    ESP_LOGI(TAG, "adc_pattern[%d].atten is :%x", i, adc_pattern[i].atten);
    ESP_LOGI(TAG, "adc_pattern[%d].channel is :%x", i, adc_pattern[i].channel);
    ESP_LOGI(TAG, "adc_pattern[%d].unit is :%x", i, adc_pattern[i].unit);
  }

  dig_cfg.adc_pattern = adc_pattern;
  ESP_ERROR_CHECK(adc_continuous_config(handle, &dig_cfg));

  *out_handle = handle;
}

void CT_Task(void *pvParameters) {
  uint32_t freq = (uint32_t)pvParameters;
  int READ_LEN = SOC_ADC_DIGI_RESULT_BYTES * no_of_channels * no_of_scans;
  esp_err_t ret;
  uint32_t ret_num = 0;
  uint8_t result[READ_LEN];
  memset(result, 0xcc, READ_LEN);

  adc_continuous_handle_t handle = NULL;
  continuous_adc_init(list_of_adc_channels,
                      sizeof(list_of_adc_channels) / sizeof(adc_channel_t),
                      &handle, READ_LEN, freq);

  int voltage[8] = {0, 0, 0, 0, 0, 0, 0, 0};
  uint16_t data[8];

  adc_continuous_evt_cbs_t cbs = {
      .on_conv_done = s_conv_done_cb,
  };
  ESP_ERROR_CHECK(adc_continuous_register_event_callbacks(handle, &cbs, NULL));
  ESP_ERROR_CHECK(adc_continuous_start(handle));

  /**
   * This is to show you the way to use the ADC continuous mode driver event
   * callback. This `ulTaskNotifyTake` will block when the data processing in
   * the task is fast. However in this example, the data processing (print) is
   * slow, so you barely block here.
   *
   * Without using this event callback (to notify this task), you can still just
   * call `adc_continuous_read()` here in a loop, with/without a certain block
   * timeout.
   */
  ulTaskNotifyTake(pdTRUE, portMAX_DELAY);

  while (1) {
    // ESP_LOGW(TAG, "Scan start");
    ret = adc_continuous_read(handle, result, READ_LEN, &ret_num, 2);
    if (ret == ESP_OK) {
      int dec = 0;
      for (int i = 0; i < ret_num;
           i += SOC_ADC_DIGI_RESULT_BYTES) // SOC_ADC_DIGI_DATA_BYTES_PER_CONV
      {
        adc_digi_output_data_t *p = (void *)&result[i];
        if (check_valid_data(p)) {
          // ESP_LOGI(TAG, "Unit: %d, Type: %d, Channel: %d, Value: %d", 1, 1,
          // SOC_ADC_GET_CHANNEL(p), SOC_ADC_GET_DATA(p));
          data[SOC_ADC_GET_CHANNEL(p)] = SOC_ADC_GET_DATA(p);
          voltage[SOC_ADC_GET_CHANNEL(p)] =
              fmap(data[SOC_ADC_GET_CHANNEL(p)], 0, 4095, 0, 2500);
          dec++;
        } else {
          ESP_LOGI(TAG, "Invalid data");
        }
        if (dec >= no_of_channels) {
          // for (int i = 0; i < 8; i++)
          // {
          //     printf("V%d: %d mV\n",i,voltage[i]);
          // }
          xQueueSend(calculator_queue_handle, voltage, pdMS_TO_TICKS(5));
          dec = 0;

          vTaskDelay(pdMS_TO_TICKS(10));
        }
      }
      /**
       * Because printing is slow, so every time you call `ulTaskNotifyTake`, it
       * will immediately return. To avoid a task watchdog timeout, add a delay
       * here. When you replace the way you process the data, usually you don't
       * need this delay (as this task will block for a while).
       */
      vTaskDelay(pdMS_TO_TICKS(1000));
    } else if (ret == ESP_ERR_TIMEOUT) {
      // We try to read `EXAMPLE_READ_LEN` until API returns timeout, which
      // means there's no available data
      break;
    }
  }

  ESP_ERROR_CHECK(adc_continuous_stop(handle));
  ESP_ERROR_CHECK(adc_continuous_deinit(handle));
}

void current_cal(void *pvParameters) {
  int voltage[8];
  double avg_mV_output[no_of_channels];
  for (int i = 0; i < no_of_channels; i++) {
    avg_mV_output[i] = 1202;
  }

  int samples = 0;
  while (true) {
    if (xQueueReceive(calculator_queue_handle, voltage, portMAX_DELAY)) {
      // printf("mV Value:");
      for (int i = 0; i < no_of_channels; i++) {
        // voltage[list_of_adc_channels[i]] = voltage[list_of_adc_channels[i]] -
        // 1202;
        voltage[list_of_adc_channels[i]] =
            voltage[list_of_adc_channels[i]] - 1168;
        // voltage[list_of_adc_channels[i]] = voltage[list_of_adc_channels[i]] -
        // 1575;
        voltage[list_of_adc_channels[i]] =
            voltage[list_of_adc_channels[i]] * voltage[list_of_adc_channels[i]];
        voltage[list_of_adc_channels[i]] =
            sqrt(voltage[list_of_adc_channels[i]]);
        avg_mV_output[i] = avg_mV_output[i] + voltage[list_of_adc_channels[i]];
        // printf(", %f", avg_mV_output[i]);
      }
      samples++;
    }
    if (samples >= no_of_scans) {
      for (int i = 0; i < no_of_channels; i++) {
        avg_mV_output[i] = avg_mV_output[i] / (no_of_scans + 1);
      }

      // printf("mV Value: %d ", samples);
      for (int i = 0; i < no_of_channels; i++) {
        switch (ct_to_encode[i]) {
        case CT_100A_1V:
          /* code */
          avg_mV_output[i] =
              fmap(avg_mV_output[i], 117, 256, 15.5, 29) * (15.8 / 15.38);
          break;
        case CT_50A_1V:
          /* code */
          break;
        case CT_30A_1V:
          /* code */
          break;
        case CT_100A_50mV:
          /* code */
          avg_mV_output[i] = fmap(avg_mV_output[i], 80, 193, 14.4, 31.7);
          break;
        default:
          break;
        }
        xQueueSend(*current_queue[i], &avg_mV_output[i], pdMS_TO_TICKS(5));
        // printf("%f\n", avg_mV_output[i]); // cal print
        // printf("\n\n\nwaterMark CT %d\n\n\n",
        // uxTaskGetStackHighWaterMark(CT_Task_Handle)); printf("\n\n\nwaterMark
        // %d\n\n\n", uxTaskGetStackHighWaterMark(NULL));
      }
      // printf("\n");
      samples = 0;
    }
  }
}

void CT_init(CT_config_t *configs) {
  no_of_channels = configs->no_of_adc_channels;
  for (int i = 0; i < configs->no_of_adc_channels; i++) {
    configs->cts[i].data_queue_handle = xQueueCreate(10, sizeof(double));
    list_of_adc_channels[i] = configs->cts[i].adc_channel;
    current_queue[i] = &configs->cts[i].data_queue_handle;
    ct_to_encode[i] = configs->cts[i].type;
  }
  xTaskCreate(CT_Task,                             /* Task function. */
              "currentLoop",                       /* name of task. */
              6000,                                /* Stack size of task */
              (void *)configs->sampling_frequency, /* parameter of the task */
              1,                                   /* priority of the task */
              &CT_Task_Handle); /* Task handle to keep track of created task */

  calculator_queue_handle = xQueueCreate(no_of_scans, sizeof(int) * 8);

  xTaskCreate(current_cal,          /* Task function. */
              "currentWaveAverage", /* name of task. */
              4000,                 /* Stack size of task */
              NULL,                 /* parameter of the task */
              1,                    /* priority of the task */
              NULL); /* Task handle to keep track of created task */
}

esp_err_t validate_config(CT_config_t *configs) {
  // Checks for Checking the sampling frequency in range.
  if (configs->sampling_frequency > SOC_ADC_SAMPLE_FREQ_THRES_HIGH ||
      configs->sampling_frequency < SOC_ADC_SAMPLE_FREQ_THRES_LOW) {
    return FREQUENCY_OUT_OF_RANGE;
  }
  if (configs->no_of_adc_channels > 8) {
    return ADC_CHANNELS_OUT_OF_RANGE;
  }
  return ESP_OK;
}
