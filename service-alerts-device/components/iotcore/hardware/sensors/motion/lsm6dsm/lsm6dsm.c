
#include "lsm6dsm.h"
#include "driver/i2c.h"
#include "esp_err.h"
#include "esp_log.h"
#include "esp_types.h"
#include "lsm6dsm_reg.h"
#include "sdkconfig.h"
#include <math.h>
#include <stdio.h>
#include <string.h>

#define SAMPLE_PERIOD_MS 200

#define I2C_SCL_IO 25
#define I2C_SDA_IO 26
#define I2C_FREQ_HZ 10000
int SENSOR_BUS = I2C_NUM_0;
#define I2C_TX_BUF_DISABLE 0
#define I2C_RX_BUF_DISABLE 0

#define WRITE_BIT I2C_MASTER_WRITE
#define READ_BIT I2C_MASTER_READ
#define ACK_CHECK_EN 0x1
#define ACK_CHECK_DIS 0x0
#define ACK_VAL 0x0
#define NACK_VAL 0x1
#define LSM6DSM_I2C_ADD_L 0xD5U
#define LSM6DSM_I2C_ADD_H 0x6BU
#define LSM6DSM_ID 0x6AU
#define LSM6DSO_ID 0x6CU

#define BOOT_TIME 15
#define WAIT_TIME_A 100
#define WAIT_TIME_G_01 150
#define WAIT_TIME_G_02 50
#define m_PI 3.14159265359
#define RAD_TO_DEG 180.0 / m_PI

/* Self test results. */
#define ST_PASS 1U
#define ST_FAIL 0U
#ifdef CONFIG_SELF_TEST_ENABLE
#define MIN_ST_LIMIT_mg 90.0f
#define MAX_ST_LIMIT_mg 1700.0f
#define MIN_ST_LIMIT_mdps 150000.0f
#define MAX_ST_LIMIT_mdps 700000.0f
#endif
#ifdef CONFIG_TAP_DETECT_ENABLE
bool TAP_DETECTION = true;
#else
bool TAP_DETECTION = false;
#endif
/* Private variables ---------------------------------------------------------*/
static uint8_t whoamI, rst;
stmdev_ctx_t dev_ctx;
lsm6dsm_int2_route_t int_2_reg;
int16_t data_raw_acceleration[3];
int16_t data_raw_angular_rate[3];
double acceleration_g[3] = {0};
double linear_acceleration;
double angular_rate_dps[3];
lsm6dsm_reg_t reg;
const int SensorReadingIntervalMillis = 1000;
const TickType_t maxDelay =
    pdMS_TO_TICKS(100); // Maximum delay of 100 milliseconds
QueueHandle_t HandleToQueue;

static int32_t platform_write(void *handle, uint8_t reg, const uint8_t *bufp,
                              uint16_t len);
static int32_t platform_read(void *handle, uint8_t reg, uint8_t *bufp,
                             uint16_t len);
static void platform_delay(uint32_t ms);
void platform_init(void);
static void update_imu_data(stmdev_ctx_t handle);
static void update_accel_data(void *handle);

static void lsm6dsm_init() {
  dev_ctx.write_reg = platform_write;
  dev_ctx.read_reg = platform_read;
  dev_ctx.handle = &SENSOR_BUS;
  lsm6dsm_reset_set(&dev_ctx, PROPERTY_ENABLE);
  do {
    lsm6dsm_reset_get(&dev_ctx, &rst);
  } while (rst);

  lsm6dsm_block_data_update_set(&dev_ctx, PROPERTY_ENABLE);
  if (TAP_DETECTION) {
    lsm6dsm_xl_data_rate_set(&dev_ctx, LSM6DSM_XL_ODR_52Hz);
    lsm6dsm_xl_full_scale_set(&dev_ctx, LSM6DSM_4g);
  } else {
    lsm6dsm_xl_data_rate_set(&dev_ctx, LSM6DSM_XL_ODR_416Hz);
    lsm6dsm_xl_full_scale_set(&dev_ctx, LSM6DSM_2g);
  }
  platform_delay(100);
  lsm6dsm_gy_data_rate_set(&dev_ctx, LSM6DSM_GY_ODR_208Hz);
  lsm6dsm_gy_full_scale_set(&dev_ctx, LSM6DSM_2000dps);
  lsm6dsm_xl_filter_analog_set(&dev_ctx, LSM6DSM_XL_ANA_BW_400Hz);
  lsm6dsm_xl_lp2_bandwidth_set(&dev_ctx, LSM6DSM_XL_LOW_NOISE_LP_ODR_DIV_100);
  lsm6dsm_gy_band_pass_set(&dev_ctx, LSM6DSM_HP_260mHz_LP1_STRONG);

  lsm6dsm_device_id_get(&dev_ctx, &whoamI);
  printf("who am i =%x \n", whoamI);
}

static int32_t platform_write(void *handle, uint8_t reg, const uint8_t *bufp,
                              uint16_t len) {
  i2c_cmd_handle_t cmd = i2c_cmd_link_create();
  i2c_master_start(cmd);
  i2c_master_write_byte(cmd, LSM6DSM_I2C_ADD_H << 1 | I2C_MASTER_WRITE,
                        ACK_CHECK_EN);
  i2c_master_write_byte(cmd, reg, ACK_CHECK_EN);
  i2c_master_write(cmd, bufp, len, ACK_CHECK_EN);
  ESP_LOG_BUFFER_HEXDUMP("i2c_write", bufp, len, ESP_LOG_DEBUG);
  i2c_master_stop(cmd);
  i2c_master_cmd_begin(SENSOR_BUS, cmd, 1000 / portTICK_PERIOD_MS);
  i2c_cmd_link_delete(cmd);
  return 0;
}

static int32_t platform_read(void *handle, uint8_t reg, uint8_t *bufp,
                             uint16_t len) {
  if (len == 0) {
    return ESP_OK;
  }
  i2c_cmd_handle_t cmd = i2c_cmd_link_create();
  i2c_master_start(cmd);
  i2c_master_write_byte(cmd, LSM6DSM_I2C_ADD_H << 1, ACK_CHECK_EN);
  i2c_master_write_byte(cmd, reg, ACK_CHECK_EN);
  i2c_master_start(cmd);
  i2c_master_write_byte(cmd, LSM6DSM_I2C_ADD_H << 1 | I2C_MASTER_READ,
                        ACK_CHECK_EN);
  if (len > 1) {
    i2c_master_read(cmd, bufp, len - 1, ACK_VAL);
  }
  i2c_master_read_byte(cmd, bufp + len - 1, ACK_VAL);
  i2c_master_stop(cmd);
  ESP_LOG_BUFFER_HEXDUMP("i2c_read_register", &reg, len, ESP_LOG_DEBUG);

  i2c_master_cmd_begin(SENSOR_BUS, cmd, 1000 / portTICK_PERIOD_MS);
  i2c_cmd_link_delete(cmd);
  return 0;
}

static void platform_delay(uint32_t ms) { vTaskDelay(ms / portTICK_PERIOD_MS); }

void platform_init(void) {
  i2c_config_t i2c_config = {.mode = I2C_MODE_MASTER,
                             .sda_io_num = I2C_SDA_IO,
                             .scl_io_num = I2C_SCL_IO,
                             .sda_pullup_en = GPIO_PULLUP_DISABLE,
                             .scl_pullup_en = GPIO_PULLUP_DISABLE,
                             .master.clk_speed = I2C_FREQ_HZ};
  i2c_param_config(SENSOR_BUS, &i2c_config);
  esp_err_t err;
  err = i2c_driver_install(SENSOR_BUS, i2c_config.mode, 0, 0, 0);
  if (err != ESP_OK)
    printf("Something wrong here\n");
}

static void update_accel_data(void *handle) {
  memset(data_raw_acceleration, 0x00, 3 * sizeof(int16_t));
  lsm6dsm_acceleration_raw_get(&dev_ctx, data_raw_acceleration);
  acceleration_g[0] = lsm6dsm_from_fs4g_to_g(data_raw_acceleration[0]);
  acceleration_g[1] = lsm6dsm_from_fs4g_to_g(data_raw_acceleration[1]);
  acceleration_g[2] = lsm6dsm_from_fs4g_to_g(data_raw_acceleration[2]);
  linear_acceleration = sqrt((acceleration_g[0] * acceleration_g[0]) +
                             (acceleration_g[1] * acceleration_g[1]) +
                             (acceleration_g[2] * acceleration_g[2])) -
                        sqrt(acceleration_g[2] * acceleration_g[2]);
}
/*
orientation[0]=X-Axis
orientation[1]=Y-Axis
orientation[2]=Z-Axis
if the acceleration to be measured is in XY axis the XY axis must be set to
false and Z axis set to true
*/
double get_linear_acceleration(bool orientation[]) {
  if (orientation[0] == false && orientation[1] == false &&
      orientation[2] == true) {
    return sqrt((acceleration_g[0] * acceleration_g[0]) +
                (acceleration_g[1] * acceleration_g[1]) +
                (acceleration_g[2] * acceleration_g[2])) -
           sqrt(acceleration_g[2] * acceleration_g[2]);
  } else if (orientation[0] == false && orientation[1] == true &&
             orientation[2] == false) {
    return sqrt((acceleration_g[0] * acceleration_g[0]) +
                (acceleration_g[1] * acceleration_g[1]) +
                (acceleration_g[2] * acceleration_g[2])) -
           sqrt(acceleration_g[1] * acceleration_g[1]);
  } else if (orientation[0] == true && orientation[1] == false &&
             orientation[2] == false) {
    return sqrt((acceleration_g[0] * acceleration_g[0]) +
                (acceleration_g[1] * acceleration_g[1]) +
                (acceleration_g[2] * acceleration_g[2])) -
           sqrt(acceleration_g[0] * acceleration_g[0]);
  } else
    return 0;
}
double get_pitch() {

  double pitch = atan2(-acceleration_g[0], acceleration_g[2]) *
                 RAD_TO_DEG; // rotation on Y axis

  return pitch;
}

double get_roll() {

  double roll = atan2(-acceleration_g[1], acceleration_g[2]) * RAD_TO_DEG;
  return roll;
}
static void update_gyro_data(void *handle) {
  memset(data_raw_angular_rate, 0x00, 3 * sizeof(int16_t));
  lsm6dsm_angular_rate_raw_get(&dev_ctx, data_raw_angular_rate);
  angular_rate_dps[0] = lsm6dsm_from_fs2000dps_to_dps(data_raw_angular_rate[0]);
  angular_rate_dps[1] = lsm6dsm_from_fs2000dps_to_dps(data_raw_angular_rate[1]);
  angular_rate_dps[2] = lsm6dsm_from_fs2000dps_to_dps(data_raw_angular_rate[2]);
}
void lsm6dsm_self_test(void) {
  uint8_t tx_buffer[1000];
  int16_t data_raw[3];
  stmdev_ctx_t dev_ctx;
  float val_st_off[3];
  float val_st_on[3];
  float test_val[3];
  uint8_t st_result;
  uint8_t whoamI;
  uint8_t drdy;
  uint8_t rst;
  uint8_t i;
  uint8_t j;
  dev_ctx.write_reg = platform_write;
  dev_ctx.read_reg = platform_read;
  dev_ctx.handle = &SENSOR_BUS;
  platform_delay(BOOT_TIME);
  lsm6dsm_device_id_get(&dev_ctx, &whoamI);

  if (whoamI != LSM6DSM_ID) {
    printf("device ID failed\n");
  }
  /* Restore default configuration */
  lsm6dsm_reset_set(&dev_ctx, PROPERTY_ENABLE);

  do {
    lsm6dsm_reset_get(&dev_ctx, &rst);
  } while (rst);

  /* Enable Block Data Update */
  lsm6dsm_block_data_update_set(&dev_ctx, PROPERTY_ENABLE);
  /*
   * Accelerometer Self Test
   */
  /* Set Output Data Rate */
  lsm6dsm_xl_data_rate_set(&dev_ctx, LSM6DSM_XL_ODR_52Hz);
  /* Set full scale */
  lsm6dsm_xl_full_scale_set(&dev_ctx, LSM6DSM_4g);
  /* Wait stable output */
  platform_delay(WAIT_TIME_A);

  /* Check if new value available */
  do {
    lsm6dsm_xl_flag_data_ready_get(&dev_ctx, &drdy);
  } while (!drdy);

  /* Read dummy data and discard it */
  lsm6dsm_acceleration_raw_get(&dev_ctx, data_raw);
  /* Read 5 sample and get the average vale for each axis */
  memset(val_st_off, 0x00, 3 * sizeof(float));

  for (i = 0; i < 5; i++) {
    /* Check if new value available */
    do {
      lsm6dsm_xl_flag_data_ready_get(&dev_ctx, &drdy);
    } while (!drdy);

    /* Read data and accumulate the mg value */
    lsm6dsm_acceleration_raw_get(&dev_ctx, data_raw);

    for (j = 0; j < 3; j++) {
      val_st_off[j] += lsm6dsm_from_fs4g_to_mg(data_raw[j]);
    }
  }

  /* Calculate the mg average values */
  for (i = 0; i < 3; i++) {
    val_st_off[i] /= 5.0f;
  }

  /* Enable Self Test positive (or negative) */
  lsm6dsm_xl_self_test_set(&dev_ctx, LSM6DSM_XL_ST_NEGATIVE);
  // lsm6dsm_xl_self_test_set(&dev_ctx, LSM6DSM_XL_ST_POSITIVE);
  /* Wait stable output */
  platform_delay(WAIT_TIME_A);

  /* Check if new value available */
  do {
    lsm6dsm_xl_flag_data_ready_get(&dev_ctx, &drdy);
  } while (!drdy);

  /* Read dummy data and discard it */
  lsm6dsm_acceleration_raw_get(&dev_ctx, data_raw);
  /* Read 5 sample and get the average vale for each axis */
  memset(val_st_on, 0x00, 3 * sizeof(float));

  for (i = 0; i < 5; i++) {
    /* Check if new value available */
    do {
      lsm6dsm_xl_flag_data_ready_get(&dev_ctx, &drdy);
    } while (!drdy);

    /* Read data and accumulate the mg value */
    lsm6dsm_acceleration_raw_get(&dev_ctx, data_raw);

    for (j = 0; j < 3; j++) {
      val_st_on[j] += lsm6dsm_from_fs4g_to_mg(data_raw[j]);
    }
  }

  /* Calculate the mg average values */
  for (i = 0; i < 3; i++) {
    val_st_on[i] /= 5.0f;
  }

  /* Calculate the mg values for self test */
  for (i = 0; i < 3; i++) {
    test_val[i] = fabs((val_st_on[i] - val_st_off[i]));
  }

  /* Check self test limit */
  st_result = ST_PASS;

  for (i = 0; i < 3; i++) {
    if ((MIN_ST_LIMIT_mg > test_val[i]) || (test_val[i] > MAX_ST_LIMIT_mg)) {
      st_result = ST_FAIL;
    }
  }

  /* Disable Self Test */
  lsm6dsm_xl_self_test_set(&dev_ctx, LSM6DSM_XL_ST_DISABLE);
  /* Disable sensor. */
  lsm6dsm_xl_data_rate_set(&dev_ctx, LSM6DSM_XL_ODR_OFF);
  /*
   * Gyroscope Self Test
   */
  /* Set Output Data Rate */
  lsm6dsm_gy_data_rate_set(&dev_ctx, LSM6DSM_GY_ODR_208Hz);
  /* Set full scale */
  lsm6dsm_gy_full_scale_set(&dev_ctx, LSM6DSM_2000dps);
  /* Wait stable output */
  platform_delay(WAIT_TIME_G_01);

  /* Check if new value available */
  do {
    lsm6dsm_gy_flag_data_ready_get(&dev_ctx, &drdy);
  } while (!drdy);

  /* Read dummy data and discard it */
  lsm6dsm_angular_rate_raw_get(&dev_ctx, data_raw);
  /* Read 5 sample and get the average vale for each axis */
  memset(val_st_off, 0x00, 3 * sizeof(float));

  for (i = 0; i < 5; i++) {
    /* Check if new value available */
    do {
      lsm6dsm_gy_flag_data_ready_get(&dev_ctx, &drdy);
    } while (!drdy);

    /* Read data and accumulate the mg value */
    lsm6dsm_angular_rate_raw_get(&dev_ctx, data_raw);

    for (j = 0; j < 3; j++) {
      val_st_off[j] += lsm6dsm_from_fs2000dps_to_mdps(data_raw[j]);
    }
  }

  /* Calculate the mg average values */
  for (i = 0; i < 3; i++) {
    val_st_off[i] /= 5.0f;
  }

  /* Enable Self Test positive (or negative) */
  lsm6dsm_gy_self_test_set(&dev_ctx, LSM6DSM_GY_ST_POSITIVE);
  // lsm6dsm_gy_self_test_set(&dev_ctx, LIS2DH12_GY_ST_NEGATIVE);
  /* Wait stable output */
  platform_delay(WAIT_TIME_G_02);
  /* Read 5 sample and get the average vale for each axis */
  memset(val_st_on, 0x00, 3 * sizeof(float));

  for (i = 0; i < 5; i++) {
    /* Check if new value available */
    do {
      lsm6dsm_gy_flag_data_ready_get(&dev_ctx, &drdy);
    } while (!drdy);

    /* Read data and accumulate the mg value */
    lsm6dsm_angular_rate_raw_get(&dev_ctx, data_raw);

    for (j = 0; j < 3; j++) {
      val_st_on[j] += lsm6dsm_from_fs2000dps_to_mdps(data_raw[j]);
    }
  }

  /* Calculate the mg average values */
  for (i = 0; i < 3; i++) {
    val_st_on[i] /= 5.0f;
  }

  /* Calculate the mg values for self test */
  for (i = 0; i < 3; i++) {
    test_val[i] = fabs((val_st_on[i] - val_st_off[i]));
  }

  /* Check self test limit */
  for (i = 0; i < 3; i++) {
    if ((MIN_ST_LIMIT_mdps > test_val[i]) ||
        (test_val[i] > MAX_ST_LIMIT_mdps)) {
      st_result = ST_FAIL;
    }
  }

  /* Disable Self Test */
  lsm6dsm_gy_self_test_set(&dev_ctx, LSM6DSM_GY_ST_DISABLE);
  /* Disable sensor. */
  lsm6dsm_gy_data_rate_set(&dev_ctx, LSM6DSM_GY_ODR_OFF);

  if (st_result == ST_PASS) {
    ESP_LOGI("IMU_SELF_TEST", "SELF_TEST_PASSED");
  }

  else {
    ESP_LOGE("IMU_SELF_TEST", "SELF_TEST_FAILED");
  }
}

static void lsm6dsm_tap_double(void) {
  /* Initialize mems driver interface */
  stmdev_ctx_t dev_ctx;
  /* Uncomment if need interrupt on Double Tap (select int1 or 2) */
  lsm6dsm_int1_route_t int_1_reg;
  // lsm6dsm_int2_route_t int_2_reg;
  dev_ctx.write_reg = platform_write;
  dev_ctx.read_reg = platform_read;
  dev_ctx.handle = &SENSOR_BUS;
  /* Wait sensor boot time */
  platform_delay(BOOT_TIME);
  /* Check device ID */
  lsm6dsm_device_id_get(&dev_ctx, &whoamI);

  if (whoamI != LSM6DSM_ID)
    while (1) {
      /* manage here device not found */
    }

  /* Restore default configuration */
  lsm6dsm_reset_set(&dev_ctx, PROPERTY_ENABLE);

  do {
    lsm6dsm_reset_get(&dev_ctx, &rst);
  } while (rst);

  /* Set XL Output Data Rate to 416 Hz */
  lsm6dsm_xl_data_rate_set(&dev_ctx, LSM6DSM_XL_ODR_416Hz);
  /* Set 2g full XL scale */
  lsm6dsm_xl_full_scale_set(&dev_ctx, LSM6DSM_2g);
  /* Enable Tap detection on X, Y, Z */
  lsm6dsm_tap_detection_on_z_set(&dev_ctx, PROPERTY_ENABLE);
  lsm6dsm_tap_detection_on_y_set(&dev_ctx, PROPERTY_ENABLE);
  lsm6dsm_tap_detection_on_x_set(&dev_ctx, PROPERTY_ENABLE);
  lsm6dsm_4d_mode_set(&dev_ctx, PROPERTY_ENABLE);
  /* Set Tap threshold to 01100b, therefore the tap threshold
   * is 750 mg (= 12 * FS_XL / 2 5 )
   */
  lsm6dsm_tap_threshold_x_set(&dev_ctx, 0x0c);
  /* Configure Double Tap parameter
   *
   * The SHOCK field of the INT_DUR2 register is set to 11b, therefore
   * the Shock time is 57.7 ms (= 3 * 8 / ODR_XL)
   *
   * The QUIET field of the INT_DUR2 register is set to 11b, therefore
   * the Quiet time is 28.8 ms (= 3 * 4 / ODR_XL)
   *
   * For the maximum time between two consecutive detected taps, the DUR
   * field of the INT_DUR2 register is set to 0111b, therefore the Duration
   * time is 538.5 ms (= 7 * 32 / ODR_XL)
   */
  lsm6dsm_tap_dur_set(&dev_ctx, 0x07);
  lsm6dsm_tap_quiet_set(&dev_ctx, 0x03);
  lsm6dsm_tap_shock_set(&dev_ctx, 0x03);
  /* Enable Double Tap detection */
  lsm6dsm_tap_mode_set(&dev_ctx, LSM6DSM_BOTH_SINGLE_DOUBLE);
  /* Enable interrupt generation on Double Tap INT1 pin */
  lsm6dsm_pin_int1_route_get(&dev_ctx, &int_1_reg);
  int_1_reg.int1_double_tap = PROPERTY_ENABLE;
  lsm6dsm_pin_int1_route_set(&dev_ctx, int_1_reg);

  /* Uncomment if interrupt generation on Double Tap INT2 pin */
  // lsm6dsm_pin_int2_route_get(&dev_ctx, &int_2_reg);
  // int_2_reg.int2_double_tap = PROPERTY_ENABLE;
  // lsm6dsm_pin_int2_route_set(&dev_ctx, int_2_reg);

  /* Wait Events */
  while (1) {
    lsm6dsm_all_sources_t all_source;
    /* Check if Double Tap events */
    lsm6dsm_all_sources_get(&dev_ctx, &all_source);
    if (all_source.tap_src.double_tap) {
      printf("Double Tap Detected\r\n");
    }

    if (all_source.tap_src.single_tap) {
      printf("Single Tap Detected\r\n");
    }
  }
}

static void update_imu_data(stmdev_ctx_t handle) {
  lsm6dsm_status_reg_get(&handle, &reg.status_reg);
  if (reg.status_reg.xlda) {
    /* Read acceleration field data */
    update_accel_data(&handle);
  } else {
    ESP_LOGW("LSM", "A status reg not set");
  }
  if (reg.status_reg.gda) {
    /* Read angular rate field data */
    update_gyro_data(&handle);
  } else {
    ESP_LOGW("LSM", "G status reg not set");
  }
}
static void lsm6dsm_measurement_task(void) {

  while (1) {
    update_imu_data(dev_ctx);
    lsm6dsm_device_id_get(&dev_ctx, &whoamI);
    // if (whoamI != LSM6DSM_ID || whoamI != LSM6DSO_ID)
    //   ESP_LOGE("LSM6DSM", "Device LSM6DSM/LSM6DSO not found");
    platform_delay(100);
  }
}
void lsm6dsm_task(void) {
#ifdef CONFIG_SELF_TEST_ENABLE
  lsm6dsm_self_test();
#endif
  lsm6dsm_init();
  xTaskCreate(lsm6dsm_measurement_task, "lsm6dsm_measurement_task", 2024 * 2,
              NULL, 10, NULL);
}

/**
 * @brief This function continouosly gets sensor readings and push them to a
 * queue
 * @param pvParameters  task parameters
 */
void Get_LSM6DSO_Data(void *pvParameters) {
  BaseType_t QueueSendCheck;
  bool orientationa[3] = {false, false, true};
  struct LSM6DSO_SensorData SD;
  while (1) {
    SD.LinearAcceleration = get_linear_acceleration(orientationa);
    SD.Pitch = get_pitch();
    SD.Roll = get_roll();

    // pushed measure value to queue
    QueueSendCheck = xQueueSend(HandleToQueue, &SD, maxDelay);
    if (QueueSendCheck != pdPASS) {
      // Queue is full, send the item to the end of the queue
      xQueueSendToBack(HandleToQueue, &SD, maxDelay);
    }

    // printf("Acceleration=%lf\npitch=%lf\nroll=%lf\n",SD.LinearAcceleration,SD.Pitch,SD.Roll);
    vTaskDelay(SensorReadingIntervalMillis / portTICK_PERIOD_MS);
  }
}

/**
 * @brief Creates a task that continously retrieves sensor readings and push
 * them into a queue
 * @param SizeOfQueue Size of queue
 * @param TaskHandle  Handle to the task being created
 * @retval  HandleToQueue -- Returns a handle to the created queue otherwise
 * returns NULL if queue is not created
 */
QueueHandle_t LSM6DSO_GetSensorReadingTask(int SizeOfQueue,
                                           TaskHandle_t *TaskHandle) {
  BaseType_t TaskCreateCheck;
  platform_init();
  lsm6dsm_task();

  HandleToQueue = xQueueCreate(SizeOfQueue, sizeof(struct LSM6DSO_SensorData));
  if (HandleToQueue != NULL) {
    ESP_LOGI("LSM6DSO Queue", "Queue cerated!");
    TaskCreateCheck =
        xTaskCreate(Get_LSM6DSO_Data, "LSM6DSO_Get_Sensor_Data_Task", 2000,
                    NULL, tskIDLE_PRIORITY + 2, TaskHandle);
    if (TaskCreateCheck == pdPASS) {
      ESP_LOGI("LSM6DSO_Task", "Task created!");
      return HandleToQueue;
    } else {
      ESP_LOGE("LSM6DSO_Task", "Task creation failed!");
      return NULL;
    }

  } else {
    ESP_LOGE("LSM6DSO_Queue", "Queue creation fialed!");
    return NULL;
  }
}