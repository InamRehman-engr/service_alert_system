
#include "max30102.h"

// globals
// Status Registers
#define MAX30102_INTSTAT1 = 0x00U
#define MAX30102_INTSTAT2 0x01U
#define MAX30102_INTENABLE1 0x02U
#define MAX30102_INTENABLE2 0x03U

// FIFO Registers
#define MAX30102_FIFOWRITEPTR 0x04U
#define MAX30102_FIFOOVERFLOW 0x05U
#define MAX30102_FIFOREADPTR 0x06U
#define MAX30102_FIFODATA 0x07U

// Configuration Registers
#define MAX30102_FIFOCONFIG 0x08U
#define MAX30102_MODECONFIG 0x09U
#define MAX30102_PARTICLECONFIG                                                \
  0x0AU // Note, sometimes listed as "SPO2" config in datasheet (pg. 11)
#define MAX30102_LED1_PULSEAMP 0x0CU
#define MAX30102_LED2_PULSEAMP 0x0DU
#define MAX30102_LED3_PULSEAMP 0x0EU
#define MAX30102_LED_PROX_AMP 0x10U
#define MAX30102_MULTILEDCONFIG1 0x11U
#define MAX30102_MULTILEDCONFIG2 0x12U

// Die Temperature Registers
#define MAX30102_DIETEMPINT 0x1FU
#define MAX30102_DIETEMPFRAC 0x20U
#define MAX30102_DIETEMPCONFIG 0x21U

// Proximity Function Registers
#define MAX30102_PROXINTTHRESH 0x30U

// Part ID Registers
#define MAX30102_REVISIONID 0xFEU
#define MAX30102_PARTID 0xFFU // Should always be 0x15. Identical to MAX30102.

// MAX30102 Commands
// Interrupt configuration (pg 13, 14)
#define MAX30102_INT_A_FULL_MASK (uint8_t) ~0b10000000U
#define MAX30102_INT_A_FULL_ENABLE 0x80U
#define MAX30102_INT_A_FULL_DISABLE 0x00U

#define MAX30102_INT_DATA_RDY_MASK (uint8_t) ~0b01000000U
#define MAX30102_INT_DATA_RDY_ENABLE 0x40U
#define MAX30102_INT_DATA_RDY_DISABLE 0x00U

#define MAX30102_INT_ALC_OVF_MASK (uint8_t) ~0b00100000U
#define MAX30102_INT_ALC_OVF_ENABLE 0x20U
#define MAX30102_INT_ALC_OVF_DISABLE 0x00U

#define MAX30102_INT_PROX_INT_MASK (uint8_t) ~0b00010000U
#define MAX30102_INT_PROX_INT_ENABLE 0x10U
#define MAX30102_INT_PROX_INT_DISABLE 0x00U

#define MAX30102_INT_DIE_TEMP_RDY_MASK (uint8_t) ~0b00000010U
#define MAX30102_INT_DIE_TEMP_RDY_ENABLE 0x02U
#define MAX30102_INT_DIE_TEMP_RDY_DISABLE 0x00U

#define MAX30102_SAMPLEAVG_MASK (uint8_t) ~0b11100000U
#define MAX30102_SAMPLEAVG_1 0x00U
#define MAX30102_SAMPLEAVG_2 0x20U
#define MAX30102_SAMPLEAVG_4 0x40U
#define MAX30102_SAMPLEAVG_8 0x60U
#define MAX30102_SAMPLEAVG_16 0x80U
#define MAX30102_SAMPLEAVG_32 0xA0U

#define MAX30102_ROLLOVER_MASK 0xEFU
#define MAX30102_ROLLOVER_ENABLE 0x10U
#define MAX30102_ROLLOVER_DISABLE 0x00U

#define MAX30102_A_FULL_MASK 0xF0U

// Mode configuration commands (page 19)
#define MAX30102_SHUTDOWN_MASK 0x7FU
#define MAX30102_SHUTDOWN 0x80U
#define MAX30102_WAKEUP 0x00U

#define MAX30102_RESET_MASK 0xBFU
#define MAX30102_RESET 0x40U

#define MAX30102_MODE_MASK 0xF8U
#define MAX30102_MODE_REDONLY 0x02U
#define MAX30102_MODE_REDIRONLY 0x03U
#define MAX30102_MODE_MULTILED 0x07U

// Particle sensing configuration commands (pgs 19-20)
#define MAX30102_ADCRANGE_MASK 0x9FU
#define MAX30102_ADCRANGE_2048 0x00U
#define MAX30102_ADCRANGE_4096 0x20U
#define MAX30102_ADCRANGE_8192 0x40U
#define MAX30102_ADCRANGE_16384 0x60U

#define MAX30102_SAMPLERATE_MASK 0xE3U
#define MAX30102_SAMPLERATE_50 0x00U
#define MAX30102_SAMPLERATE_100 0x04U
#define MAX30102_SAMPLERATE_200 0x08U
#define MAX30102_SAMPLERATE_400 0x0CU
#define MAX30102_SAMPLERATE_800 0x10U
#define MAX30102_SAMPLERATE_1000 0x14U
#define MAX30102_SAMPLERATE_1600 0x18U
#define MAX30102_SAMPLERATE_3200 0x1CU

#define MAX30102_PULSEWIDTH_MASK 0xFCU
#define MAX30102_PULSEWIDTH_69 0x00U
#define MAX30102_PULSEWIDTH_118 0x01U
#define MAX30102_PULSEWIDTH_215 0x02U
#define MAX30102_PULSEWIDTH_411 0x03U

// Multi-LED Mode configuration (pg 22)
#define MAX30102_SLOT1_MASK 0xF8U
#define MAX30102_SLOT2_MASK 0x8FU
#define MAX30102_SLOT3_MASK 0xF8U
#define MAX30102_SLOT4_MASK 0x8FU

#define SLOT_NONE 0x00U
#define SLOT_RED_LED 0x01U
#define SLOT_IR_LED 0x02U
#define SLOT_GREEN_LED 0x03U
#define SLOT_NONE_PILOT 0x04U
#define SLOT_RED_PILOT 0x05U
#define SLOT_IR_PILOT 0x06U
#define SLOT_GREEN_PILOT 0x07U

#define MAX_30102_EXPECTEDPARTID 0x15U
bool runonce = false;
bool run_once = true;
uint32_t irBuffer[100];  // infrared LED sensor data
uint32_t redBuffer[100]; // red LED sensor data
int32_t bufferLength = 100;
int32_t bufferLength; // data length
int32_t spo2;         // SPO2 value
int8_t validSPO2;     // indicator to show if the SPO2 calculation is valid
int32_t heartRate;    // heart rate value
int8_t
    validHeartRate; // indicator to show if the heart rate calculation is valid
esp_err_t i2c_init() {
  i2c_config_t conf = {.mode = I2C_MODE_MASTER,
                       .sda_io_num = i2c_gpio_sda,
                       .sda_pullup_en = GPIO_PULLUP_ENABLE,
                       .scl_io_num = i2c_gpio_scl,
                       .scl_pullup_en = GPIO_PULLUP_ENABLE,
                       .master.clk_speed = i2c_frequency};
  i2c_param_config(i2c_port, &conf);
  return i2c_driver_install(i2c_port, I2C_MODE_MASTER,
                            I2C_MASTER_RX_BUF_DISABLE,
                            I2C_MASTER_TX_BUF_DISABLE, 0);
}

static int i2c_read(uint8_t chip_addr, uint8_t data_addr, uint8_t *data_rd,
                    size_t len) {
  // i2c_init();
  // vTaskDelay(1);
  // i2c_driver_install(i2c_port, I2C_MODE_MASTER, I2C_MASTER_RX_BUF_DISABLE,
  //                    I2C_MASTER_TX_BUF_DISABLE, 0);
  i2c_cmd_handle_t cmd = i2c_cmd_link_create();
  i2c_master_start(cmd);
  // ESP_LOGI("", "Read Add addr %X  data %X", data_addr, *data_rd);
  i2c_master_write_byte(cmd, chip_addr << 1 | WRITE_BIT, ACK_CHECK_EN);
  i2c_master_write_byte(cmd, data_addr, ACK_CHECK_EN);
  i2c_master_start(cmd);
  i2c_master_write_byte(cmd, chip_addr << 1 | READ_BIT, ACK_CHECK_EN);
  if (len > 1) {
    i2c_master_read(cmd, data_rd, len - 1, ACK_VAL);
  }
  i2c_master_read_byte(cmd, data_rd + len - 1, NACK_VAL);
  i2c_master_stop(cmd);
  esp_err_t ret = i2c_master_cmd_begin(i2c_port, cmd, 100 / portTICK_PERIOD_MS);
  i2c_cmd_link_delete(cmd);
  if (ret == ESP_OK) {
    // for (int i = 0; i < len; i++)
    // {
    //     printf("%d %x\n", i, data_rd[i]);
    // }
  } else if (ret == ESP_ERR_TIMEOUT) {
    ESP_LOGW("", "Bus is busy");
  } else {
    ESP_LOGW("", "Read failed");
  }
  // i2c_driver_delete(i2c_port);
  vTaskDelay(2);
  return 0;
}

static int i2c_write(uint8_t chip_addr, uint8_t data_addr, uint8_t wr_data) {
  // i2c_init();
  // i2c_driver_install(i2c_port, I2C_MODE_MASTER, I2C_MASTER_RX_BUF_DISABLE,
  //                    I2C_MASTER_TX_BUF_DISABLE, 0);
  i2c_cmd_handle_t cmd = i2c_cmd_link_create();
  i2c_master_start(cmd);
  i2c_master_write_byte(cmd, chip_addr << 1 | WRITE_BIT, ACK_CHECK_EN);
  i2c_master_write_byte(cmd, data_addr, ACK_CHECK_EN);
  i2c_master_write_byte(cmd, wr_data, ACK_CHECK_EN);
  i2c_master_stop(cmd);
  esp_err_t ret = i2c_master_cmd_begin(i2c_port, cmd, 100 / portTICK_PERIOD_MS);
  i2c_cmd_link_delete(cmd);
  if (ret == ESP_OK) {
    ESP_LOGI("", "Write OK addr %x  data %x\n", data_addr, wr_data);
  } else if (ret == ESP_ERR_TIMEOUT) {
    ESP_LOGW("", "Bus is busy");
  } else {
    ESP_LOGW("", "Write Failed");
  }
  // i2c_driver_delete(i2c_port);
  return 0;
}

// void max30102_init()
// {
//     uint8_t data;
//     data = (0x7 << 5); // sample averaging 0=1,1=2,2=4,3=8,4=16,5+=32
//     i2c_write(I2C_ADDR_MAX30102, MAX30102_REG_FIFO_CONFIG, data);
//     data = MAX30102_REG_MODE_CONFIG_MODE_HR_SPO2; // mode = red and ir
//     samples i2c_write(I2C_ADDR_MAX30102, MAX30102_REG_MODE_CONFIG, data);
//     data = (0x3 << 5) + (MAX30102_REG_SPO2_CONFIG_SR_400HZ << 2) + 0x3; //
//     first and last 0x3, middle smap rate 0=50,1=100,etc
//     i2c_write(I2C_ADDR_MAX30102, MAX30102_REG_SPO2_CONFIG, data);
//     data = 0xd0; // ir pulse power
//     i2c_write(I2C_ADDR_MAX30102, MAX30102_REG_RED_LED_CONFIG, data);
//     data = 0xa0; // red pulse power
//     i2c_write(I2C_ADDR_MAX30102, MAX30102_REG_IR_LED_CONFIG, data);
// }

void max30102_task() {
  while (1) {
    if (run_once == true) {
      bufferLength = 100;
      for (uint8_t i = 0; i < bufferLength; i++) {
        while (available() == false) {
          check();
          vTaskDelay(10 / portTICK_PERIOD_MS);
        } // do we have new data?
        // Check the sensor for new data
        redBuffer[i] = getRed();
        irBuffer[i] = getIR();
        // ESP_LOGI("", "redbuffer[%d]=%d", i, redBuffer[i]);
        // ESP_LOGI("", "irbuffer[%d]=%d", i, irBuffer[i]);
        nextSample(); // We're finished with this sample so move to next sample
      }
      ESP_LOG_BUFFER_HEXDUMP("RED=", redBuffer, 100, ESP_LOG_WARN);
      ESP_LOG_BUFFER_HEXDUMP("IR=", irBuffer, 100, ESP_LOG_WARN);
      vTaskDelay(10 / portTICK_PERIOD_MS);
      maxim_heart_rate_and_oxygen_saturation(irBuffer, bufferLength, redBuffer,
                                             &spo2, &validSPO2, &heartRate,
                                             &validHeartRate);
      run_once = false;
    }
    while (1) {
      // dumping the first 25 sets of samples in the memory and shift the last
      // 75 sets of samples to the top
      for (uint8_t i = 25; i < 100; i++) {
        redBuffer[i - 25] = redBuffer[i];
        irBuffer[i - 25] = irBuffer[i];
      }

      // take 25 sets of samples before calculating the heart rate.
      for (uint8_t i = 75; i < 100; i++) {
        while (available() == false) // do we have new data?
          check();                   // Check the sensor for new data

        redBuffer[i] = getRed();
        irBuffer[i] = getIR();
        nextSample(); // We're finished with this sample so move to next sample

        printf("HR=%ld", heartRate);

        printf(", HRvalid=%d", validHeartRate);

        printf(", SPO2=%ld", spo2);

        printf(", SPO2Valid=%d\n", validSPO2);
      }
      maxim_heart_rate_and_oxygen_saturation(irBuffer, bufferLength, redBuffer,
                                             &spo2, &validSPO2, &heartRate,
                                             &validHeartRate);
    }
  }
}
void max30102_setup(uint8_t powerLevel, uint8_t sampleAverage, uint8_t ledMode,
                    int sampleRate, int pulseWidth, int adcRange) {
  max30102_softReset();
  // FIFO Configuration
  //-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
  // The chip will average multiple samples of same type together if you wish
  if (sampleAverage == 1)
    setFIFOAverage(MAX30102_SAMPLEAVG_1); // No averaging per FIFO record
  else if (sampleAverage == 2)
    setFIFOAverage(MAX30102_SAMPLEAVG_2);
  else if (sampleAverage == 4) {
    setFIFOAverage(MAX30102_SAMPLEAVG_4);
  } else if (sampleAverage == 8)
    setFIFOAverage(MAX30102_SAMPLEAVG_8);
  else if (sampleAverage == 16)
    setFIFOAverage(MAX30102_SAMPLEAVG_16);
  else if (sampleAverage == 32)
    setFIFOAverage(MAX30102_SAMPLEAVG_32);
  else
    setFIFOAverage(MAX30102_SAMPLEAVG_4);
  enableFIFORollover();
  // Mode Configuration
  //-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
  if (ledMode == 3)
    setLEDMode(MAX30102_MODE_MULTILED); // Watch all three LED channels
  else if (ledMode == 2)
    setLEDMode(MAX30102_MODE_REDIRONLY); // Red and IR
  else
    setLEDMode(MAX30102_MODE_REDONLY); // Red only
  activeLEDs = ledMode; // Used to control how many bytes to read from FIFO
                        // buffer Particle Sensing Configuration
  //-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
  if (adcRange < 4096)
    setADCRange(MAX30102_ADCRANGE_2048); // 7.81pA per LSB
  else if (adcRange < 8192)
    setADCRange(MAX30102_ADCRANGE_4096); // 15.63pA per LSB
  else if (adcRange < 16384)
    setADCRange(MAX30102_ADCRANGE_8192); // 31.25pA per LSB
  else if (adcRange == 16384)
    setADCRange(MAX30102_ADCRANGE_16384); // 62.5pA per LSB
  else
    setADCRange(MAX30102_ADCRANGE_2048);

  if (sampleRate < 100)
    setSampleRate(MAX30102_SAMPLERATE_50); // Take 50 samples per second
  else if (sampleRate < 200)
    setSampleRate(MAX30102_SAMPLERATE_100);
  else if (sampleRate < 400)
    setSampleRate(MAX30102_SAMPLERATE_200);
  else if (sampleRate < 800)
    setSampleRate(MAX30102_SAMPLERATE_400);
  else if (sampleRate < 1000)
    setSampleRate(MAX30102_SAMPLERATE_800);
  else if (sampleRate < 1600)
    setSampleRate(MAX30102_SAMPLERATE_1000);
  else if (sampleRate < 3200)
    setSampleRate(MAX30102_SAMPLERATE_1600);
  else if (sampleRate == 3200)
    setSampleRate(MAX30102_SAMPLERATE_3200);
  else
    setSampleRate(MAX30102_SAMPLERATE_50);

  // The longer the pulse width the longer range of detection you'll have
  // At 69us and 0.4mA it's about 2 inches
  // At 411us and 0.4mA it's about 6 inches
  if (pulseWidth < 118)
    setPulseWidth(MAX30102_PULSEWIDTH_69); // Page 26, Gets us 15 bit resolution
  else if (pulseWidth < 215)
    setPulseWidth(MAX30102_PULSEWIDTH_118); // 16 bit resolution
  else if (pulseWidth < 411)
    setPulseWidth(MAX30102_PULSEWIDTH_215); // 17 bit resolution
  else if (pulseWidth == 411)
    setPulseWidth(MAX30102_PULSEWIDTH_411); // 18 bit resolution
  else
    setPulseWidth(MAX30102_PULSEWIDTH_69);
  //-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

  // LED Pulse Amplitude Configuration
  //-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
  // Default is 0x1F which gets us 6.4mA
  // powerLevel = 0x02, 0.4mA - Presence detection of ~4 inch
  // powerLevel = 0x1F, 6.4mA - Presence detection of ~8 inch
  // powerLevel = 0x7F, 25.4mA - Presence detection of ~8 inch
  // powerLevel = 0xFF, 50.0mA - Presence detection of ~12 inch

  setPulseAmplitudeRed(powerLevel);
  setPulseAmplitudeIR(powerLevel);
  setPulseAmplitudeGreen(powerLevel);
  setPulseAmplitudeProximity(powerLevel);
  //-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

  // Multi-LED Mode Configuration, Enable the reading of the three LEDs
  //-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
  enableSlot(1, SLOT_RED_LED);
  if (ledMode > 1)
    enableSlot(2, SLOT_IR_LED);
  if (ledMode > 2)
    enableSlot(3, SLOT_GREEN_LED);
  // enableSlot(1, SLOT_RED_PILOT);
  // enableSlot(2, SLOT_IR_PILOT);
  // enableSlot(3, SLOT_GREEN_PILOT);
  //-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

  clearFIFO(); // Reset the FIFO before we begin checking the sensor
  ESP_LOGI("MAX_INIT", "INIT SUCESSFUL");
}
void max30102_softReset(void) {
  bitMask(MAX30102_MODECONFIG, MAX30102_RESET_MASK, MAX30102_RESET);

  // Poll for bit to clear, reset is then complete
  // Timeout after 100ms
  unsigned long startTime = ((unsigned long)esp_timer_get_time()) / 1000;
  uint8_t response = 0;
  while (((unsigned long)(esp_timer_get_time()) / 1000) - startTime < 100) {

    i2c_read(I2C_ADDR_MAX30102, MAX30102_MODECONFIG, &response, 1);
    if ((response & MAX30102_RESET) == 0)
      break;                            // We're done!
    vTaskDelay(1 / portTICK_PERIOD_MS); // Let's not over burden the I2C bus
  }
}
void bitMask(uint8_t reg, uint8_t mask, uint8_t thing) {
  // Grab current register context
  uint8_t originalContents = 0;
  i2c_read(I2C_ADDR_MAX30102, reg, &originalContents, 1);
  // Zero-out the portions of the register we're interested in
  originalContents = originalContents & mask;

  // Change contents
  i2c_write(I2C_ADDR_MAX30102, reg, originalContents | thing);
}
// Enable roll over if FIFO over flows
void enableFIFORollover(void) {
  bitMask(MAX30102_FIFOCONFIG, MAX30102_ROLLOVER_MASK,
          MAX30102_ROLLOVER_ENABLE);
}
//
// FIFO Configuration
//

// Set sample average (Table 3, Page 18)
void setFIFOAverage(uint8_t numberOfSamples) {
  bitMask(MAX30102_FIFOCONFIG, MAX30102_SAMPLEAVG_MASK, numberOfSamples);
}
void setLEDMode(uint8_t mode) {
  // Set which LEDs are used for sampling -- Red only, RED+IR only, or custom.
  // See datasheet, page 19
  bitMask(MAX30102_MODECONFIG, MAX30102_MODE_MASK, mode);
}
void setADCRange(uint8_t adcRange) {
  // adcRange: one of MAX30102_ADCRANGE_2048, _4096, _8192, _16384
  bitMask(MAX30102_PARTICLECONFIG, MAX30102_ADCRANGE_MASK, adcRange);
}
void setSampleRate(uint8_t sampleRate) {
  // sampleRate: one of MAX30102_SAMPLERATE_50, _100, _200, _400, _800, _1000,
  // _1600, _3200
  bitMask(MAX30102_PARTICLECONFIG, MAX30102_SAMPLERATE_MASK, sampleRate);
}
void setPulseWidth(uint8_t pulseWidth) {
  // pulseWidth: one of MAX30102_PULSEWIDTH_69, _188, _215, _411
  bitMask(MAX30102_PARTICLECONFIG, MAX30102_PULSEWIDTH_MASK, pulseWidth);
}
// NOTE: Amplitude values: 0x00 = 0mA, 0x7F = 25.4mA, 0xFF = 50mA (typical)
// See datasheet, page 21
void setPulseAmplitudeRed(uint8_t amplitude) {
  i2c_write(I2C_ADDR_MAX30102, MAX30102_LED1_PULSEAMP, amplitude);
}

void setPulseAmplitudeIR(uint8_t amplitude) {
  i2c_write(I2C_ADDR_MAX30102, MAX30102_LED2_PULSEAMP, amplitude);
}

void setPulseAmplitudeGreen(uint8_t amplitude) {
  i2c_write(I2C_ADDR_MAX30102, MAX30102_LED3_PULSEAMP, amplitude);
}

void setPulseAmplitudeProximity(uint8_t amplitude) {
  i2c_write(I2C_ADDR_MAX30102, MAX30102_LED_PROX_AMP, amplitude);
}
void enableSlot(uint8_t slotNumber, uint8_t device) {
  switch (slotNumber) {
  case (1):
    bitMask(MAX30102_MULTILEDCONFIG1, MAX30102_SLOT1_MASK, device);
    break;
  case (2):
    bitMask(MAX30102_MULTILEDCONFIG1, MAX30102_SLOT2_MASK, device << 4);
    break;
  case (3):
    bitMask(MAX30102_MULTILEDCONFIG2, MAX30102_SLOT3_MASK, device);
    break;
  case (4):
    bitMask(MAX30102_MULTILEDCONFIG2, MAX30102_SLOT4_MASK, device << 4);
    break;
  default:
    // Shouldn't be here!
    break;
  }
}
void clearFIFO(void) {
  i2c_write(I2C_ADDR_MAX30102, MAX30102_FIFOWRITEPTR, 0);
  i2c_write(I2C_ADDR_MAX30102, MAX30102_FIFOOVERFLOW, 0);
  i2c_write(I2C_ADDR_MAX30102, MAX30102_FIFOREADPTR, 0);
}
uint8_t available(void) {
  int8_t numberOfSamples = sense.head - sense.tail;
  if (numberOfSamples < 0)
    numberOfSamples += STORAGE_SIZE;

  return (numberOfSamples);
}

uint16_t check(void) {
  // Read register FIDO_DATA in (3-byte * number of active LED) chunks
  // Until FIFO_RD_PTR = FIFO_WR_PTR
  uint8_t regdata[256];
  uint8_t readPointer = getReadPointer();
  uint8_t writePointer = getWritePointer();

  int numberOfSamples = 0;
  int i = 0;
  // Do we have new data?
  if (readPointer != writePointer) {
    // Calculate the number of readings we need to get from sensor
    numberOfSamples = writePointer - readPointer;
    if (numberOfSamples < 0)
      numberOfSamples += 32; // Wrap condition

    // We now have the number of readings, now calc bytes to read
    // For this example we are just doing Red and IR (3 bytes each)
    int bytesLeftToRead = numberOfSamples * activeLEDs * 3;

    // We may need to read as many as 288 bytes so we read in blocks no larger
    // than I2C_BUFFER_LENGTH I2C_BUFFER_LENGTH changes based on the platform.
    // 64 bytes for SAMD21, 32 bytes for Uno. Wire.requestFrom() is limited to
    // BUFFER_LENGTH which is 32 on the Uno
    while (bytesLeftToRead > 0) {
      int toGet = bytesLeftToRead;
      if (toGet > I2C_BUFFER_LENGTH) {
        // If toGet is 32 this is bad because we read 6 bytes (Red+IR * 3 = 6)
        // at a time 32 % 6 = 2 left over. We don't want to request 32 bytes, we
        // want to request 30. 32 % 9 (Red+IR+GREEN) = 5 left over. We want to
        // request 27.

        toGet =
            I2C_BUFFER_LENGTH -
            (I2C_BUFFER_LENGTH %
             (activeLEDs *
              3)); // Trim toGet to be a multiple of the samples we need to read
      }

      bytesLeftToRead -= toGet;

      // Request toGet number of bytes from sensor
      i2c_read(I2C_ADDR_MAX30102, MAX30102_FIFODATA, regdata, toGet);
      // ESP_LOG_BUFFER_HEXDUMP("Regdata=", regdata, toGet, ESP_LOG_INFO);
      while (toGet > 0) {
        sense.head++;               // Advance the head of the storage struct
        sense.head %= STORAGE_SIZE; // Wrap condition

        uint8_t temp[sizeof(
            uint32_t)]; // Array of 4 bytes that we will convert into long
        uint32_t tempLong;

        // Burst read three bytes - RED
        temp[3] = 0;
        temp[2] = regdata[i];
        temp[1] = regdata[i + 1];
        temp[0] = regdata[i + 2];

        // Convert array to long
        memcpy(&tempLong, temp, sizeof(tempLong));

        tempLong &= 0x3FFFF; // Zero out all but 18 bits
        printf("RED=%lld ", (long long)tempLong);
        sense.red[sense.head] =
            tempLong; // Store this reading into the sense array

        if (activeLEDs > 1) {
          // Burst read three more bytes - IR
          temp[3] = 0;
          temp[2] = regdata[i + 3];
          temp[1] = regdata[i + 4];
          temp[0] = regdata[i + 5];

          // Convert array to long
          memcpy(&tempLong, temp, sizeof(tempLong));

          tempLong &= 0x3FFFF; // Zero out all but 18 bits
          printf("IR=%lld\n", (long long)tempLong);
          sense.IR[sense.head] = tempLong;
        }

        toGet -= activeLEDs * 3;
        i += 3;
        vTaskDelay(10 / portTICK_PERIOD_MS);
        // printf("In to get\n");
      }
      vTaskDelay(10 / portTICK_PERIOD_MS);
    } // End while (bytesLeftToRead > 0)
      // printf("In to readPtr != writePtr\n");
  }   // End readPtr != writePtr

  return (numberOfSamples); // Let the world know how much new data we found
  printf("number of samples=%d", numberOfSamples);
}
uint8_t getWritePointer(void) {
  uint8_t wptr;
  i2c_read(I2C_ADDR_MAX30102, MAX30102_FIFOWRITEPTR, &wptr, 1);
  return wptr;
}

// Read the FIFO Read Pointer
uint8_t getReadPointer(void) {
  uint8_t rptr;
  i2c_read(I2C_ADDR_MAX30102, MAX30102_FIFOREADPTR, &rptr, 1);
  return rptr;
}

uint32_t getRed(void) {
  // Check the sensor for new data for 250ms
  // if (safeCheck(250))
  return (sense.red[sense.head]);
  //  else
  //  {
  //      printf("Returning RED\n");
  //      return (0); // Sensor failed to find new data
  //  }
}

// Report the most recent IR value
uint32_t getIR(void) {
  // Check the sensor for new data for 250ms
  // if (safeCheck(250))
  return (sense.IR[sense.head]);
  //  else
  //  {
  //      printf("Returning IR\n");
  //      return (0); // Sensor failed to find new data
  //  }
}

bool safeCheck(uint8_t maxTimeToCheck) {
  uint32_t markTime = ((unsigned long)esp_timer_get_time()) / 1000;

  while (1) {
    if (((unsigned long)(esp_timer_get_time()) / 1000) - markTime >
        maxTimeToCheck)
      return (false);

    if (check() == true) // We found new data!
      return (true);

    vTaskDelay(1 / portTICK_PERIOD_MS);
  }
}

// Advance the tail
void nextSample(void) {
  if (available()) // Only advance the tail if new data is available
  {
    sense.tail++;
    sense.tail %= STORAGE_SIZE; // Wrap condition
  }
}
void app_max30102(void) {
  esp_err_t err;
  uint8_t ledBrightness = 60; // Options: 0=Off to 255=50mA
  uint8_t sampleAverage = 4;  // Options: 1, 2, 4, 8, 16, 32
  uint8_t ledMode =
      2; // Options: 1 = Red only, 2 = Red + IR, 3 = Red + IR + Green
  uint8_t sampleRate = 100; // Options: 50, 100, 200, 400, 800, 1000, 1600, 3200
  int pulseWidth = 411;     // Options: 69, 118, 215, 411
  int adcRange = 4096;      // Options: 2048, 4096, 8192, 16384

  if (runonce == false) {
    err = i2c_init();
    if (err != ESP_OK) {
      ESP_LOGW("I2C_INIT", "INIT FAILED");
    } else
      ESP_LOGI("I2C_INIT", "INIT SUCESSFUL");
    max30102_setup(ledBrightness, sampleAverage, ledMode, sampleRate,
                   pulseWidth,
                   adcRange); // Configure sensor with these settings
    runonce = true;
  }

  // data collection tasks
  xTaskCreate(max30102_task, "max30102_task", 4096 * 8, NULL, 5, NULL);
}