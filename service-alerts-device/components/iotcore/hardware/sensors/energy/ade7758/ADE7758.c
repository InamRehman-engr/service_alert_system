#include "ADE7758.h"

#include "driver/gpio.h"
#include "errorHandling.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/task.h"
#include "nvs_read_write.h"
#include <spi_master_dev.h>

static const char *TAG = "ADE7758";

QueueHandle_t calibration_queue = NULL;
const char *voltage_key[] = {"AVRMSOS", "BVRMSOS", "CVRMSOS"};
const char *current_key[] = {"AIRMSOS", "BIRMSOS", "CIRMSOS"};

esp_err_t SPI_Read_Generic(uint8_t addr, uint8_t *data, size_t len,
                           spi_master_functions *ADE7758) {
  uint8_t Send_TX[4] = {0}; // Assuming a maximum of 4 bytes for transmission
  uint8_t Rec_RX[4] = {0};  // Adjust size if more bytes are expected
  if (ADE7758->send_receive(ADE7758, 0, SPI_REG_READ(addr), Send_TX, Rec_RX,
                            len) == ESP_OK) {
    memcpy(data, Rec_RX,
           len); // Copy the received data into the provided buffer
    return ESP_OK;
  } else {
    ESP_LOGE(TAG, "SPI Read error");
    return ESP_FAIL;
  }
}

uint8_t Read_8bit(uint8_t addr, spi_master_functions *ADE7758) {
  uint8_t buffer = 0;
  SPI_Read_Generic(addr, &buffer, 1, ADE7758);
  return buffer;
}

uint32_t Read_24bit(uint8_t addr, spi_master_functions *ADE7758) {
  uint8_t buffer[3] = {0};
  SPI_Read_Generic(addr, buffer, 3, ADE7758);
  return (buffer[0] << 16) | (buffer[1] << 8) | buffer[2];
}

esp_err_t Write_8bit(uint8_t addr, uint8_t data,
                     spi_master_functions *ADE7758) {
  uint8_t Send_TX[4] = {0};
  Send_TX[0] = data;
  return ADE7758->send(ADE7758, 0, SPI_REG_WRITE(addr), Send_TX, 1);
}

esp_err_t Write_16bit(uint8_t addr, uint16_t data,
                      spi_master_functions *ADE7758) {
  uint8_t Send_TX[4] = {0};
  Send_TX[0] = (uint8_t)((data & 0xFF00) >> 8);
  Send_TX[1] = (uint8_t)(data & 0x00FF);
  return ADE7758->send(ADE7758, 0, SPI_REG_WRITE(addr), Send_TX, 2);
}

esp_err_t Write_24bit(uint8_t addr, uint32_t data,
                      spi_master_functions *ADE7758) {
  uint8_t Send_TX[4] = {0};
  Send_TX[0] = (uint8_t)((data & 0xFF0000) >> 16);
  Send_TX[1] = (uint8_t)((data & 0x00FF00) >> 8);
  Send_TX[2] = (uint8_t)(data & 0x00FF);
  return ADE7758->send(ADE7758, 0, SPI_REG_WRITE(addr), Send_TX, 3);
}

int32_t get_raw_current(ade7758_handle_t *handle, uint8_t phase) {
  /**
   * the rms registers are next to each other so adding he phase
   * number to the 1st register will give us the next register
   * since phase starts from 0 to higher number.
   *
   */
  return Read_24bit(AIRMS + phase, &handle->spi);
}

int32_t get_raw_voltage(ade7758_handle_t *handle, uint8_t phase) {
  /**
   * the rms registers are next to each other so adding he phase
   * number to the 1st register will give us the next register
   * since phase starts from 0 to higher number.
   *
   */
  return Read_24bit(AVRMS + phase, &handle->spi);
}

void average_raw_data(ade7758_handle_t *handle, double *val, uint8_t samples,
                      uint8_t phase, uint8_t mode) {
  for (int j = 0; j < (phase != ALL_PHASE ? 1 : 3); j++) {
    for (int i = 0; i < samples; i++) {
      val[j] +=
          (mode == VOLTAGE)
              ? get_raw_voltage(&handle->spi, phase != ALL_PHASE ? phase : j)
              : get_raw_current(&handle->spi, phase != ALL_PHASE ? phase : j);
    }
    val[j] /= samples;
  }
}

float get_current(ade7758_handle_t *handle, double factor, uint8_t phase) {
  double current = 0;
  if (phase == ALL_PHASE) {
    ESP_LOGW(TAG, "Phase ALL_PHASE Current = NaN");
    ESP_LOGE(TAG, "ALL_PHASE is not a valid phase");
    return 0;
  }
  average_raw_data(handle, &current, 10, phase, CURRENT);
  current *= factor;
  ESP_LOGI(TAG, "Phase %c Current = %f", ('A' + phase), (float)current);
  if (current < 0.15) {
    current = 0;
  }
  return (float)current;
}

uint8_t read_opmode(ade7758_handle_t *handle) {
  uint8_t reg_data;
  reg_data = Read_8bit(OPMODE, &handle->spi);
  ESP_LOGI(TAG, "OPMODE = 0x%02X", reg_data);
  return reg_data;
}

// Set ADE7758 ADC opmode
esp_err_t set_adc_opmode(ade7758_handle_t *handle, ade7758_opmode dismod) {
  uint8_t opmode = read_opmode(handle);
  opmode = (opmode & 0xC7) | (dismod << 3); // Set DISMOD for ADE7758 ADCs
  return (Write_8bit(OPMODE, opmode, &handle->spi));
}

float get_voltage(ade7758_handle_t *handle, double factor, uint8_t phase) {
  double voltage;
  if (phase == ALL_PHASE) {
    ESP_LOGW(TAG, "Phase ALL_PHASE Voltage = NaN");
    ESP_LOGE(TAG, "ALL_PHASE is not a valid phase");
    return 0;
  }
  average_raw_data(handle, &voltage, 10, phase, VOLTAGE);
  voltage *= factor;
  ESP_LOGI(TAG, "Phase %c Voltage = %f", ('A' + phase), voltage);
  if (voltage < 0.15) {
    voltage = 0;
  }
  return (float)voltage;
}

bool send_ade7758_calibration_message(ade7758_calibration_comands_t *commands) {
  if (calibration_queue == NULL) {
    return false;
  }
  xQueueSend(calibration_queue, commands, 0);
  return true;
}

void calculate_factor(double *raw_val, double scale_val, uint8_t phase) {
  if (phase != ALL_PHASE) {
    raw_val[phase] = scale_val / raw_val[phase];
  } else {
    raw_val[PHASE_A] = scale_val / raw_val[PHASE_A];
    raw_val[PHASE_B] = scale_val / raw_val[PHASE_B];
    raw_val[PHASE_C] = scale_val / raw_val[PHASE_C];
  }
}

void Voffset_init(spi_master_functions *ADE7758,
                  ade7758_calibration_response_t *response, uint8_t phase,
                  float Vnom, float Vmin, double *Vrmsmin, double *Vrmsnom) {
  int16_t vrmsos = 0;
  if (phase != ALL_PHASE) {
    vrmsos = calculate_Voffset(Vnom, Vmin, Vrmsmin[phase], Vrmsnom[phase]);
    vrmsos = vrmsos & 0x0FFF;
    Write_16bit(AVRMSOS + phase, vrmsos, ADE7758);
    saveKeyValueInFlash_int32(voltage_key[phase], vrmsos);
    memcpy(&response->Voffset[phase], &vrmsos, sizeof(uint16_t));
    ESP_LOGI(TAG, "Voltage Offset %c Data: 0x%04X", 'A' + phase, vrmsos);
  } else {
    Voffset_init(ADE7758, response, PHASE_A, Vnom, Vmin, Vrmsmin, Vrmsnom);
    Voffset_init(ADE7758, response, PHASE_B, Vnom, Vmin, Vrmsmin, Vrmsnom);
    Voffset_init(ADE7758, response, PHASE_C, Vnom, Vmin, Vrmsmin, Vrmsnom);
  }
}

void Ioffset_init(spi_master_functions *ADE7758,
                  ade7758_calibration_response_t *response, uint8_t phase,
                  float Itest, float Imin, double *Irmsmin, double *Irmstest) {
  int16_t irmsos = 0;
  if (phase != ALL_PHASE) {
    irmsos = calculate_Ioffset(Itest, Imin, Irmsmin[phase], Irmstest[phase]);
    irmsos = irmsos & 0x0FFF;
    Write_16bit(AVRMSOS + phase, irmsos, ADE7758);
    saveKeyValueInFlash_int32(current_key[phase], irmsos);
    memcpy(&response->ioffset[phase], &irmsos, sizeof(uint16_t));
    ESP_LOGI(TAG, "Current Offset %c Data: 0x%04X", 'A' + phase, irmsos);
  } else {
    Ioffset_init(ADE7758, response, PHASE_A, Itest, Imin, Irmsmin, Irmstest);
    Ioffset_init(ADE7758, response, PHASE_B, Itest, Imin, Irmsmin, Irmstest);
    Ioffset_init(ADE7758, response, PHASE_C, Itest, Imin, Irmsmin, Irmstest);
  }
}

void reset_IOS(spi_master_functions *ADE7758, uint8_t phase) {
  if (phase != ALL_PHASE) {
    Write_24bit(AIRMSOS + phase, 0, ADE7758);
  } else {
    reset_IOS(ADE7758, PHASE_A);
    reset_IOS(ADE7758, PHASE_B);
    reset_IOS(ADE7758, PHASE_C);
  }
}

void reset_VOS(spi_master_functions *ADE7758, uint8_t phase) {
  if (phase != ALL_PHASE) {
    Write_24bit(AVRMSOS + phase, 0, ADE7758);
  } else {
    reset_VOS(ADE7758, PHASE_A);
    reset_VOS(ADE7758, PHASE_B);
    reset_VOS(ADE7758, PHASE_C);
  }
}

/// TODO: Test Voltage calibration
void ade7758_calibration_task(void *pvParameters) {
  ade7758_handle_t *handle = (ade7758_handle_t *)pvParameters;
  bool cal_not_done = true;
  calibration_queue = xQueueCreate(10, sizeof(ade7758_calibration_comands_t));
  ade7758_calibration_comands_t message = {0};
  ade7758_calibration_response_t *response =
      malloc(sizeof(ade7758_calibration_response_t));
  double min[3] = {0};
  double test_nom[3] = {0};
  double factor[3] = {0};
  bool cal_done[4] = {false};
  float pre_val = 0;
  while (cal_not_done) {
    if (xQueueReceive(calibration_queue, &message, portMAX_DELAY)) {
      if ((message.type == CALIBRATE_ITEST) &&
          !(cal_done[CALIBRATE_VNOM] || cal_done[CALIBRATE_VMIN] ||
            cal_done[CALIBRATE_ITEST])) { // condition is designed to prevent
                                          // re-execution of collection of same
                                          // value, and execution of voltage
                                          // offset calibration.
        !cal_done[CALIBRATE_IMIN] ? reset_IOS(&handle->spi, message.phase) : 0;
        average_raw_data(handle, test_nom, message.samples, message.phase,
                         CURRENT);
        cal_done[CALIBRATE_ITEST] = true;
        if (cal_done[CALIBRATE_IMIN] == true) {
          Ioffset_init(&handle->spi, response, message.phase, message.itest,
                       pre_val, test_nom, min);
          SET_RESPONSE_TYPE(RESPONSE_IOFFSET, response, message);
          ESP_LOGI(TAG, "Ioffset calibration complete");
          HANDLE_RESPONSE_CALL(handle, response);
          cal_not_done = false;
        } else {
          ESP_LOGI(TAG, "Itest Collected");
          SET_RESPONSE_TYPE(RESPONSE_ITEST, response, message);
          message.phase == ALL_PHASE
              ? memcpy(response->irmstest, test_nom, sizeof(test_nom))
              : memcpy(&response->irmstest[message.phase],
                       &test_nom[message.phase],
                       sizeof(test_nom[message.phase]));
          HANDLE_RESPONSE_CALL(handle, response);
          pre_val = message.itest;
        }
      } else if ((message.type == CALIBRATE_IMIN) &&
                 !(cal_done[CALIBRATE_VNOM] || cal_done[CALIBRATE_VMIN] ||
                   cal_done[CALIBRATE_IMIN])) { // condition is designed to
                                                // prevent re-execution of
                                                // collection of same value, and
                                                // execution of voltage offset
                                                // calibration.
        !cal_done[CALIBRATE_ITEST] ? reset_IOS(&handle->spi, message.phase) : 0;
        average_raw_data(handle, min, message.samples, message.phase, CURRENT);
        cal_done[CALIBRATE_IMIN] = true;
        if (cal_done[CALIBRATE_ITEST] == true) {
          Ioffset_init(&handle->spi, response, message.phase, pre_val,
                       message.imin, min, test_nom);
          SET_RESPONSE_TYPE(RESPONSE_IOFFSET, response, message);
          ESP_LOGI(TAG, "Ioffset calibration complete");
          HANDLE_RESPONSE_CALL(handle, response);
          cal_not_done = false;
        } else {
          ESP_LOGI(TAG, "Imin Collected");
          SET_RESPONSE_TYPE(RESPONSE_IMIN, response, message);
          message.phase == ALL_PHASE
              ? memcpy(response->irmsmin, min, sizeof(min))
              : memcpy(&response->irmsmin[message.phase], &min[message.phase],
                       sizeof(min[message.phase]));
          HANDLE_RESPONSE_CALL(handle, response);
          pre_val = message.imin;
        }
      } else if ((message.type == CALIBRATE_VNOM) &&
                 !(cal_done[CALIBRATE_ITEST] || cal_done[CALIBRATE_IMIN] ||
                   cal_done[CALIBRATE_VNOM])) { // condition is designed to
                                                // prevent re-execution of
                                                // collection of same value, and
                                                // execution of Current offset
                                                // calibration.
        !cal_done[CALIBRATE_VMIN] ? reset_VOS(&handle->spi, message.phase) : 0;
        average_raw_data(handle, test_nom, message.samples, message.phase,
                         VOLTAGE);
        cal_done[CALIBRATE_VNOM] = true;
        if (cal_done[CALIBRATE_VMIN] == true) {
          Voffset_init(&handle->spi, response, message.phase, message.vnom,
                       pre_val, min, test_nom);
          SET_RESPONSE_TYPE(RESPONSE_VOFFSET, response, message);
          ESP_LOGI(TAG, "Voffset calibration complete");
          HANDLE_RESPONSE_CALL(handle, response);
          cal_not_done = false;
        } else {
          ESP_LOGI(TAG, "Vnom Collected");
          SET_RESPONSE_TYPE(RESPONSE_VNOM, response, message);
          message.phase == ALL_PHASE
              ? memcpy(response->vrmsnom, test_nom, sizeof(test_nom))
              : memcpy(&response->vrmsnom[message.phase],
                       &test_nom[message.phase],
                       sizeof(test_nom[message.phase]));
          HANDLE_RESPONSE_CALL(handle, response);
          pre_val = message.vnom;
        }
      } else if ((message.type == CALIBRATE_VMIN) &&
                 !(cal_done[CALIBRATE_ITEST] || cal_done[CALIBRATE_IMIN] ||
                   cal_done[CALIBRATE_VMIN])) { // condition is designed to
                                                // prevent re-execution of
                                                // collection of same value, and
                                                // execution of Current offset
                                                // calibration.
        !cal_done[CALIBRATE_VNOM] ? reset_VOS(&handle->spi, message.phase) : 0;
        average_raw_data(handle, min, message.samples, message.phase, VOLTAGE);
        cal_done[CALIBRATE_VMIN] = true;
        if (cal_done[CALIBRATE_VNOM] == true) {
          Voffset_init(&handle->spi, response, message.phase, pre_val,
                       message.vmin, min, test_nom);
          SET_RESPONSE_TYPE(RESPONSE_VOFFSET, response, message);
          ESP_LOGI(TAG, "Voffset calibration complete");
          HANDLE_RESPONSE_CALL(handle, response);
          cal_not_done = false;
        } else {
          ESP_LOGI(TAG, "Vmin Collected");
          SET_RESPONSE_TYPE(RESPONSE_VMIN, response, message);
          message.phase == ALL_PHASE
              ? memcpy(response->vrmsmin, min, sizeof(min))
              : memcpy(&response->vrmsmin[message.phase], &min[message.phase],
                       sizeof(min[message.phase]));
          HANDLE_RESPONSE_CALL(handle, response);
          pre_val = message.vmin;
        }
      } else if ((message.type == CALIBRATE_ISCALE_FACTOR) &&
                 !(cal_done[CALIBRATE_ITEST] || cal_done[CALIBRATE_IMIN] ||
                   cal_done[CALIBRATE_VNOM] ||
                   cal_done[CALIBRATE_VMIN])) { // condition is designed to
                                                // prevent scaling factor
                                                // callibration from executing
                                                // if one of the offsets
                                                // callibration is in progress
        double current_factor[3] = {1, 1, 1};
        size_t cal_in_size = sizeof(handle->calibration_data.current_factor);
        readKeyValueInFlash_blob(
            "current_factor",
            (uint8_t *)handle->calibration_data.current_factor, &cal_in_size);
        average_raw_data(handle, factor, message.samples, message.phase,
                         CURRENT);
        calculate_factor(factor, message.i_factor, message.phase);
        SET_RESPONSE_TYPE(RESPONSE_ISCALE_FACTOR, response, message);
        if (message.phase != ALL_PHASE) {
          handle->calibration_data.current_factor[message.phase] =
              factor[message.phase];
        } else {
          memcpy(handle->calibration_data.current_factor, factor,
                 sizeof(factor));
        }
        ESP_LOGI(TAG, "I scalling complete");
        HANDLE_RESPONSE_CALL(handle, response);
        saveKeyValueInFlash_blob(
            "current_factor",
            (uint8_t *)handle->calibration_data.current_factor, cal_in_size);
        cal_not_done = false;
      } else if ((message.type == CALIBRATE_VSCALE_FACTOR) &&
                 !(cal_done[CALIBRATE_ITEST] || cal_done[CALIBRATE_IMIN] ||
                   cal_done[CALIBRATE_VNOM] ||
                   cal_done[CALIBRATE_VMIN])) { // condition is designed to
                                                // prevent scaling factor
                                                // callibration from executing
                                                // if one of the offsets
                                                // callibration is in progress
        double voltage_factor[3] = {1, 1, 1};
        size_t cal_in_size = sizeof(handle->calibration_data.voltage_factor);
        readKeyValueInFlash_blob(
            "voltage_factor",
            (uint8_t *)handle->calibration_data.voltage_factor, &cal_in_size);
        average_raw_data(handle, factor, message.samples, message.phase,
                         VOLTAGE);
        calculate_factor(factor, message.v_factor, message.phase);
        SET_RESPONSE_TYPE(RESPONSE_VSCALE_FACTOR, response, message);
        if (message.phase != ALL_PHASE) {
          handle->calibration_data.voltage_factor[message.phase] =
              factor[message.phase];
        } else {
          memcpy(handle->calibration_data.voltage_factor, factor,
                 sizeof(factor));
        }
        ESP_LOGI(TAG, "V scalling complete");
        HANDLE_RESPONSE_CALL(handle, response);
        saveKeyValueInFlash_blob("voltage_factor", (uint8_t *)voltage_factor,
                                 cal_in_size);
        cal_not_done = false;
      } else {
        ESP_LOGW(TAG, "Incorrect message");
        response->status = false;
        HANDLE_RESPONSE_CALL(handle, response);
      }
    }
    memset(response, 0, sizeof(ade7758_calibration_response_t));
    memset(&message, 0, sizeof(ade7758_calibration_comands_t));
  }
  free(response);
  vQueueDelete(calibration_queue);
  calibration_queue = NULL;
  handle->is_calibration_running = false;
  vTaskDelete(NULL);
}

esp_err_t start_ade_calibration(ade7758_handle_t *handle) {
  if (handle->is_calibration_running == true)
    return ESP_FAIL;
  handle->is_calibration_running = true;
  if (xTaskCreate(ade7758_calibration_task, "ade7758_calibration_task",
                  5 * 1024, handle, 6, NULL) == pdPASS) {
    return ESP_OK;
  }
  post_task_create_failed_event(__FILE__, __LINE__, esp_get_free_heap_size());
  return ESP_OK;
}

int8_t get_temperature(ade7758_handle_t *handle) {
  return Read_8bit(TEMP, &handle->spi) + 70;
}

void ade7758_task(void *pvParameters) {
  ade7758_handle_t *handle = (ade7758_handle_t *)pvParameters;

  size_t cal_in_size = sizeof(handle->calibration_data.current_factor);
  if (readKeyValueInFlash_blob(
          "current_factor", (uint8_t *)handle->calibration_data.current_factor,
          &cal_in_size) != ESP_OK) { // default callibration values
    handle->calibration_data.current_factor[PHASE_A] = 0.000055214;
    handle->calibration_data.current_factor[PHASE_B] = 0.000055214;
    handle->calibration_data.current_factor[PHASE_C] = 0.000055214;
  }

  cal_in_size = sizeof(handle->calibration_data.voltage_factor);
  if (readKeyValueInFlash_blob(
          "voltage_factor", (uint8_t *)handle->calibration_data.voltage_factor,
          &cal_in_size) !=
      ESP_OK) { /// TODO : voltage scalling values need to be calculated.
    handle->calibration_data.voltage_factor[PHASE_A] = 1;
    handle->calibration_data.voltage_factor[PHASE_B] = 1;
    handle->calibration_data.voltage_factor[PHASE_C] = 1;
  }
  while (true) {
    handle->power_save_mode ? set_adc_opmode(handle, SWITCH_OFF_VOLTAGE_ADCS)
                            : 0;
    vTaskDelay(pdMS_TO_TICKS(1000));
    handle->data.current[PHASE_A] = get_current(
        handle, handle->calibration_data.current_factor[PHASE_A], PHASE_A);
    handle->data.current[PHASE_B] = get_current(
        handle, handle->calibration_data.current_factor[PHASE_B], PHASE_B);
    handle->data.current[PHASE_C] = get_current(
        handle, handle->calibration_data.current_factor[PHASE_C], PHASE_C);
    handle->data.temperature = get_temperature(handle);
    handle->power_save_mode ? set_adc_opmode(handle, POWER_DOWN_MODE) : 0;
    vTaskDelay(pdMS_TO_TICKS(handle->update_interval_ms - 1000));
  }
  vTaskDelete(NULL);
}

void ade7758_init(ade7758_handle_t *handle) {
  if (handle == NULL)
    return;

  handle->is_calibration_running = false;
  handle->start_calibration = start_ade_calibration;
  handle->send_calibration_message = send_ade7758_calibration_message;
  Write_8bit(GAIN, 0x00, &handle->spi);   // page 19 THEORY OF OPERATION
  Write_8bit(OPMODE, 0x04, &handle->spi); // table 14 of datasheet, page 61

  Write_8bit(LCYCMODE, 0x7f,
             &handle->spi); // table 18 of datasheet, page 64

  Write_24bit(MASK, 0x00, &handle->spi); // interupt mask page 65
  int32_t rmsos = 0;
  // current os
  for (int i = 0; i < 3; i++) {
    readKeyValueInFlash_int32(current_key[i], &rmsos) == ESP_OK
        ? 0
        : (rmsos = 0xF67);
    Write_16bit(AIRMSOS + i, rmsos, &handle->spi);
    rmsos = 0;
  }
  // voltage os
  for (int i = 0; i < 3; i++) {
    readKeyValueInFlash_int32(voltage_key[i], &rmsos) == ESP_OK
        ? 0
        : (rmsos = 0x000); /// TODO : calculate standard offset values for
                           /// voltage and initialize os register with it.
    Write_16bit(AVRMSOS + i, rmsos, &handle->spi);
    rmsos = 0;
  }

  if (xTaskCreate(ade7758_task, "ade7758_task", 5 * 1024, handle, 10, NULL) !=
      pdPASS) {
    post_task_create_failed_event(__FILE__, __LINE__, esp_get_free_heap_size());
  }
}
