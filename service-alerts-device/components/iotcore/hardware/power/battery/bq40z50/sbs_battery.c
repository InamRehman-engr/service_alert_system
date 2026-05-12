#include "sbs_battery.h"
SemaphoreHandle_t read_write_mutex;
#define DELAY_READ 50
struct battery_params {
  uint8_t addr;
  int32_t min_value;
  int32_t max_value;
} battery_params_list[] = {
    [BAT_AT_RATE_TIME_TO_FULL] = SBS_DATA(0x05, 0, 65535),
    [BAT_AT_RATE_TIME_TO_EMPTY] = SBS_DATA(0x06, 0, 65535),
    [BAT_TEMPERATURE] = SBS_DATA(0x08, 0, 65535),
    [BAT_VOLTAGE] = SBS_DATA(0x09, 0, 65535),
    [BAT_CURRENT] = SBS_DATA(0x0A, -32767, 32768),
    [BAT_AVERAGE_CURRENT] = SBS_DATA(0x0B, -32767, 32768),
    [BAT_RELATIVE_STATE_OF_CHARGE] = SBS_DATA(0x0D, 0, 100),
    [BAT_ABSOLUTE_STATE_OF_CHARGE] = SBS_DATA(0x0E, 0, 100),
    [BAT_REMAINING_CAPACITY] = SBS_DATA(0x0F, 0, 65535),
    [BAT_FULL_CHARGE_CAPACITY] = SBS_DATA(0x10, 0, 65535),
    [BAT_RUN_TIME_TO_EMPTY] = SBS_DATA(0x11, 0, 65535),
    [BAT_AVERAGE_TIME_TO_EMPTY] = SBS_DATA(0x12, 0, 65535),
    [BAT_AVERAGE_TIME_TO_FULL] = SBS_DATA(0x13, 0, 65535),
    [BAT_CHARGING_CURRENT] = SBS_DATA(0x14, 0, 65535),
    [BAT_CHARGING_VOLTAGE] = SBS_DATA(0x15, 0, 65535),
    [BAT_BATTERY_STATUS] = SBS_DATA(0x16, 0, 0),
    [BAT_CYCLE_COUNT] = SBS_DATA(0x17, 0, 65535),
    [BAT_DESIGN_CAPACITY] = SBS_DATA(0x18, 0, 65535),
    [BAT_DESIGN_VOLTAGE] = SBS_DATA(0x19, 0, 65535),
    [BAT_MANUFACTURER_DATE] = SBS_DATA(0x1B, 0, 65535),
    [BAT_SERIAL_NUMBER] = SBS_DATA(0x1C, 0, 65535),
    [BAT_MANUFACTURER_NAME] = SBS_DATA(0x20, 0, 0), // Block
    [BAT_DEVICE_NAME] = SBS_DATA(0x21, 0, 0),       // Block
    [BAT_DEVICE_CHEMISTRY] = SBS_DATA(0x22, 0, 0),  // Block
    [BAT_CELL_VOLTAGE_4] = SBS_DATA(0x3C, 0, 65535),
    [BAT_CELL_VOLTAGE_3] = SBS_DATA(0x3D, 0, 65535),
    [BAT_CELL_VOLTAGE_2] = SBS_DATA(0x3E, 0, 65535),
    [BAT_CELL_VOLTAGE_1] = SBS_DATA(0x3F, 0, 65535),
    [BAT_STATE_OF_HEALTH] = SBS_DATA(0x4F, 0, 100),
    [BAT_TURBO_POWER] = SBS_DATA(0x59, 0, 65535),
    [BAT_TURBO_CURRENT] = SBS_DATA(0x5D, 0, 65535)};

esp_err_t read_critical_data(i2c_functions *i2c_obj, battery_data *data) {
  esp_err_t err = ESP_OK;
  err |= i2c_obj->smbus_receive_word(0x0B,
                                     battery_params_list[BAT_TEMPERATURE].addr,
                                     &data->temperature, i2c_obj->device);
  vTaskDelay(pdMS_TO_TICKS(DELAY_READ));
  err |=
      i2c_obj->smbus_receive_word(0x0B, battery_params_list[BAT_VOLTAGE].addr,
                                  &data->voltage, i2c_obj->device);
  vTaskDelay(pdMS_TO_TICKS(DELAY_READ));
  err |=
      i2c_obj->smbus_receive_word(0x0B, battery_params_list[BAT_CURRENT].addr,
                                  &data->current, i2c_obj->device);
  vTaskDelay(pdMS_TO_TICKS(DELAY_READ));
  err |= i2c_obj->smbus_receive_word(
      0x0B, battery_params_list[BAT_RELATIVE_STATE_OF_CHARGE].addr,
      &data->relative_state_of_charge, i2c_obj->device);
  vTaskDelay(pdMS_TO_TICKS(DELAY_READ));
  err |= i2c_obj->smbus_receive_word(
      0x0B, battery_params_list[BAT_ABSOLUTE_STATE_OF_CHARGE].addr,
      &data->absolute_state_of_charge, i2c_obj->device);
  vTaskDelay(pdMS_TO_TICKS(DELAY_READ));
  err |= i2c_obj->smbus_receive_word(
      0x0B, battery_params_list[BAT_CHARGING_CURRENT].addr,
      &data->charging_current, i2c_obj->device);
  vTaskDelay(pdMS_TO_TICKS(DELAY_READ));
  err |= i2c_obj->smbus_receive_word(
      0x0B, battery_params_list[BAT_CHARGING_VOLTAGE].addr,
      &data->charging_voltage, i2c_obj->device);
  vTaskDelay(pdMS_TO_TICKS(DELAY_READ));
  err |= i2c_obj->smbus_receive_word(
      0x0B, battery_params_list[BAT_CELL_VOLTAGE_1].addr, &data->voltage_cell_1,
      i2c_obj->device);
  vTaskDelay(pdMS_TO_TICKS(DELAY_READ));
  err |= i2c_obj->smbus_receive_word(
      0x0B, battery_params_list[BAT_CELL_VOLTAGE_2].addr, &data->voltage_cell_2,
      i2c_obj->device);
  vTaskDelay(pdMS_TO_TICKS(DELAY_READ));
  err |= i2c_obj->smbus_receive_word(
      0x0B, battery_params_list[BAT_CELL_VOLTAGE_3].addr, &data->voltage_cell_3,
      i2c_obj->device);
  vTaskDelay(pdMS_TO_TICKS(DELAY_READ));
  err |= i2c_obj->smbus_receive_word(
      0x0B, battery_params_list[BAT_CELL_VOLTAGE_4].addr, &data->voltage_cell_4,
      i2c_obj->device);

  return err;
}
esp_err_t read_less_critical_data(i2c_functions *i2c_obj, battery_data *data) {
  esp_err_t err = ESP_OK;
  err |= i2c_obj->smbus_receive_word(
      0x0B, battery_params_list[BAT_BATTERY_STATUS].addr, &data->battery_status,
      i2c_obj->device);
  vTaskDelay(pdMS_TO_TICKS(DELAY_READ));
  err |= i2c_obj->smbus_receive_word(
      0x0B, battery_params_list[BAT_AVERAGE_TIME_TO_EMPTY].addr,
      &data->average_time_to_empty, i2c_obj->device);
  vTaskDelay(pdMS_TO_TICKS(DELAY_READ));
  err |= i2c_obj->smbus_receive_word(
      0x0B, battery_params_list[BAT_AVERAGE_TIME_TO_FULL].addr,
      &data->average_time_to_full, i2c_obj->device);
  vTaskDelay(pdMS_TO_TICKS(DELAY_READ));
  err |= i2c_obj->smbus_receive_word(
      0x0B, battery_params_list[BAT_RUN_TIME_TO_EMPTY].addr,
      &data->run_time_to_empty, i2c_obj->device);
  vTaskDelay(pdMS_TO_TICKS(DELAY_READ));
  err |= i2c_obj->smbus_receive_word(
      0x0B, battery_params_list[BAT_AT_RATE_TIME_TO_FULL].addr,
      &data->at_rate_time_to_full, i2c_obj->device);
  vTaskDelay(pdMS_TO_TICKS(DELAY_READ));
  err |= i2c_obj->smbus_receive_word(
      0x0B, battery_params_list[BAT_AT_RATE_TIME_TO_EMPTY].addr,
      &data->at_rate_time_to_empty, i2c_obj->device);
  vTaskDelay(pdMS_TO_TICKS(DELAY_READ));
  err |= i2c_obj->smbus_receive_word(0x0B,
                                     battery_params_list[BAT_TURBO_POWER].addr,
                                     &data->turbo_power, i2c_obj->device);
  vTaskDelay(pdMS_TO_TICKS(DELAY_READ));
  err |= i2c_obj->smbus_receive_word(
      0x0B, battery_params_list[BAT_TURBO_CURRENT].addr, &data->turbo_current,
      i2c_obj->device);
  vTaskDelay(pdMS_TO_TICKS(DELAY_READ));
  err |= i2c_obj->smbus_receive_word(
      0x0B, battery_params_list[BAT_AVERAGE_CURRENT].addr,
      &data->average_current, i2c_obj->device);
  vTaskDelay(pdMS_TO_TICKS(DELAY_READ));
  err |= i2c_obj->smbus_receive_bytes(
      0x0B, battery_params_list[BAT_STATE_OF_HEALTH].addr,
      &data->state_of_health, 1, i2c_obj->device);
  vTaskDelay(pdMS_TO_TICKS(DELAY_READ));
  err |= i2c_obj->smbus_receive_word(
      0x0B, battery_params_list[BAT_REMAINING_CAPACITY].addr,
      &data->remaining_capacity, i2c_obj->device);
  return err;
}
esp_err_t read_only_once_data(i2c_functions *i2c_obj, battery_data *data) {
  esp_err_t err = ESP_OK;
  err |= i2c_obj->smbus_receive_word(
      0x0B, battery_params_list[BAT_SERIAL_NUMBER].addr, &data->serial_number,
      i2c_obj->device);
  vTaskDelay(pdMS_TO_TICKS(DELAY_READ));
  err |= i2c_obj->smbus_receive_word(
      0x0B, battery_params_list[BAT_MANUFACTURER_DATE].addr,
      &data->manufacturer_date, i2c_obj->device);
  vTaskDelay(pdMS_TO_TICKS(DELAY_READ));
  err |= i2c_obj->smbus_receive_word(0x0B,
                                     battery_params_list[BAT_CYCLE_COUNT].addr,
                                     &data->cycle_count, i2c_obj->device);
  vTaskDelay(pdMS_TO_TICKS(DELAY_READ));
  err |= i2c_obj->smbus_receive_word(
      0x0B, battery_params_list[BAT_DESIGN_CAPACITY].addr,
      &data->design_capacity, i2c_obj->device);
  vTaskDelay(pdMS_TO_TICKS(DELAY_READ));
  err |= i2c_obj->smbus_receive_word(
      0x0B, battery_params_list[BAT_DESIGN_VOLTAGE].addr, &data->design_voltage,
      i2c_obj->device);
  vTaskDelay(pdMS_TO_TICKS(DELAY_READ));
  err |= i2c_obj->smbus_receive_block(
      0x0B, battery_params_list[BAT_MANUFACTURER_NAME].addr,
      (uint8_t *)data->manufacturer_name, sizeof(data->manufacturer_name),
      i2c_obj->device);
  vTaskDelay(pdMS_TO_TICKS(DELAY_READ));
  err |= i2c_obj->smbus_receive_block(
      0x0B, battery_params_list[BAT_DEVICE_NAME].addr,
      (uint8_t *)data->device_name, sizeof(data->device_name), i2c_obj->device);
  vTaskDelay(pdMS_TO_TICKS(DELAY_READ));
  err |= i2c_obj->smbus_receive_block(
      0x0B, battery_params_list[BAT_DEVICE_CHEMISTRY].addr,
      (uint8_t *)data->device_chemistry, sizeof(data->device_chemistry),
      i2c_obj->device);
  vTaskDelay(pdMS_TO_TICKS(DELAY_READ));
  err |= i2c_obj->smbus_receive_word(
      0x0B, battery_params_list[BAT_FULL_CHARGE_CAPACITY].addr,
      &data->full_charge_capacity, i2c_obj->device);
  return err;
}
void read_battery_data(void *pvParams) {
  i2c_functions *i2c_obj = malloc(sizeof(i2c_functions));
  i2c_obj->device = malloc(sizeof(i2c_device_t));
  i2c_obj->device->port = I2C_NUM_0;
  i2c_init(I2C_MODE_MASTER, CONFIG_BUS1_I2C_MASTER_SDA,
           CONFIG_BUS1_I2C_MASTER_SCL, CONFIG_BUS1_SDA_PULLUP_EN,
           CONFIG_BUS1_SCL_PULLUP_EN, 50000, i2c_obj);
  esp_err_t ret = i2c_obj->device_available(0x0B, i2c_obj->device);
  battery_data *data = (battery_data *)pvParams;
  if (xSemaphoreTake(read_write_mutex, pdMS_TO_TICKS(500)) == pdTRUE) {
    read_only_once_data(i2c_obj, data);
    xSemaphoreGive(read_write_mutex);
  }
  if (xSemaphoreTake(read_write_mutex, pdMS_TO_TICKS(500)) == pdTRUE) {
    read_less_critical_data(i2c_obj, data);
    xSemaphoreGive(read_write_mutex);
  }
  int64_t start_time = esp_timer_get_time();
  while (1) {
    int64_t current_time = esp_timer_get_time();
    int64_t elapsed_time =
        (current_time - start_time) / 1000000; // Convert to seconds

    if (elapsed_time >= 30) {
      if (xSemaphoreTake(read_write_mutex, pdMS_TO_TICKS(500)) == pdTRUE) {
        read_less_critical_data(i2c_obj, data);
        xSemaphoreGive(read_write_mutex);
      }
      start_time = current_time;
    }
    if (xSemaphoreTake(read_write_mutex, pdMS_TO_TICKS(500)) == pdTRUE) {
      read_critical_data(i2c_obj, data);
      xSemaphoreGive(read_write_mutex);
    }
    vTaskDelay(pdMS_TO_TICKS(1000)); // Sleep for 1 second
  }
}

esp_err_t sbs_battery_init(battery_data *data, SemaphoreHandle_t *mutex_lock) {
  read_write_mutex = *mutex_lock;
  if (xTaskCreate(read_battery_data, "Read Battery Data", 2048, (void *)data, 3,
                  NULL) != NULL) {
    return ESP_OK;
  } else {
    return ESP_ERR_NOT_FINISHED;
  }
}