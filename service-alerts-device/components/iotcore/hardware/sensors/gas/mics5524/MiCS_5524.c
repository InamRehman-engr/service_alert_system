// work with 5volts
// normal values around 150mV
#include "MiCS_5524.h"

// ADC Channels
// ADC Attenuation
#define ADC_EXAMPLE_ATTEN ADC_ATTEN_DB_12

// ADC Calibration
#define ADC_EXAMPLE_CALI_SCHEME ESP_ADC_CAL_VAL_EFUSE_VREF
esp_err_t ret = ESP_OK;
bool cali_enable = false;
static int adc_raw[1][10];
static const uint sample = 150;
int value1, value2, val_diff, final;
static const char *TAG = "ADC Output";
uint32_t voltage1 = 0;
uint32_t voltage2 = 0;
const int SensorReadingIntervalMillis = 1000;
const TickType_t maxDelay =
    pdMS_TO_TICKS(100); // Maximum delay of 100 milliseconds
QueueHandle_t HandleToQueue;

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
  if (CONFIG_MiCS_5524_ADC_UNIT == 1)
  // ADC1 config
  {
    ESP_ERROR_CHECK(adc1_config_width(ADC_WIDTH_BIT_DEFAULT));
    ESP_ERROR_CHECK(adc1_config_channel_atten(CONFIG_MiCS_5524_ADC_CHANNEL,
                                              ADC_EXAMPLE_ATTEN));
  } else {
    // ADC2 config
    ESP_ERROR_CHECK(adc2_config_channel_atten(CONFIG_MiCS_5524_ADC_CHANNEL,
                                              ADC_EXAMPLE_ATTEN));
  }
  return ESP_OK;
}
void get_MiCS_5524(void *pvParameters) {
  BaseType_t QueueSendCheck;
  uint avg = 0;
  while (1) {
    avg = 0;
    for (int a = 0; a < sample; a++) {
      if (CONFIG_MiCS_5524_ADC_UNIT == 1) {
        adc_raw[0][0] = adc1_get_raw(CONFIG_MiCS_5524_ADC_CHANNEL);
      } else {
        do {
          ret = adc2_get_raw(CONFIG_MiCS_5524_ADC_CHANNEL,
                             ADC_WIDTH_BIT_DEFAULT, &adc_raw[0][0]);
        } while (ret == ESP_ERR_INVALID_STATE);
        ESP_ERROR_CHECK(ret);
      }

      if (cali_enable) {
        voltage1 = esp_adc_cal_raw_to_voltage(adc_raw[0][0], &adc_chars);
      }
      avg += voltage1;
    }
    final = avg / sample;
    ESP_LOGI(TAG, "Final data: %d mV", final);

    // push the measured value to queue
    QueueSendCheck = xQueueSend(HandleToQueue, &final, maxDelay);
    if (QueueSendCheck != pdPASS) {
      // Queue is full, send the item to the end of the queue
      xQueueSendToBack(HandleToQueue, &final, maxDelay);
    }
    vTaskDelay(SensorReadingIntervalMillis / portTICK_PERIOD_MS);
  }
}

/**
 * @brief   Start a freeRTOS task which periodically gets the sensor reading
 * @param   SizeOfQueue The size of queue in which the sensor readings will be
 * pushed
 * @param   TaskHandle Handle to the task being created for getting sensor
 * readings
 * @retval  HandleToQueue - Handle to queue in which readings will be pushed or
 * NULL if either queue or task creation fails
 */
QueueHandle_t GetReadingFromMiCSTask(int SizeOfQueue,
                                     TaskHandle_t *TaskHandle) {
  BaseType_t TaskCreateCheck;
  adc_calibration_init();
  ADC_init();

  HandleToQueue = xQueueCreate(
      SizeOfQueue, sizeof(int)); // create a queue for saving sensor readings
  if (HandleToQueue == NULL)     // return NULL is queue creation fails
  {
    ESP_LOGE("MICS_Queue", "Queue creation failed!");
    return NULL;
  } else {
    ESP_LOGI("MICS_Queue", "Queue created!");
    TaskCreateCheck = xTaskCreate(get_MiCS_5524, "MICS_Reading_Get_Task", 2000,
                                  NULL, tskIDLE_PRIORITY + 1, TaskHandle);
    if (TaskCreateCheck == pdPASS) {
      ESP_LOGI("MICS_Task", "MICS task created!");
      return HandleToQueue;
    } else {
      ESP_LOGE("MICS_Task", "MICS task creation failed!");
      return NULL;
    }
  }
}