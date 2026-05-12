/**************************************************************************/
/*!
    @file     INA219.c
    @author   K.Townsend (Adafruit Industries)
        @license  BSD (see license.txt)

        Driver for the INA219 current sensor

        This is a library for the Adafruit INA219 breakout
        ----> https://www.adafruit.com/products/???

        Adafruit invests time and resources providing this open source code,
        please support Adafruit and open-source hardware by purchasing
        products from Adafruit!

        @section  HISTORY

    v1.0 - First release
*/
/**************************************************************************/

#include "INA219.h"
#include "esp_err.h"
#include "freertos/projdefs.h"
#include "sdkconfig.h"

static const char *TAG = "INA219";

static esp_err_t wireWriteRegister(INA219_instance_t *instance, uint8_t reg,
                                   uint16_t value);
static esp_err_t wireReadRegister(INA219_instance_t *instance, uint8_t reg,
                                  uint16_t *value);
static esp_err_t INA219_getBusVoltage_raw(INA219_instance_t *instance,
                                          int16_t *value);
static esp_err_t INA219_getShuntVoltage_raw(INA219_instance_t *instance,
                                            int16_t *value);
static esp_err_t INA219_getCurrent_raw(INA219_instance_t *instance,
                                       int16_t *current);

#define _I2C_NUMBER(num) I2C_NUM_##num
#define I2C_NUMBER(num) _I2C_NUMBER(num)

#define I2C_MASTER_SCL_IO 19        /*!< gpio number for I2C master clock */
#define I2C_MASTER_SDA_IO 21        /*!< gpio number for I2C master data  */
#define I2C_MASTER_NUM I2C_NUM_0    /*!< I2C port number for master dev */
#define I2C_MASTER_FREQ_HZ 100000   /*!< I2C master clock frequency */
#define I2C_MASTER_TX_BUF_DISABLE 0 /*!< I2C master doesn't need buffer */
#define I2C_MASTER_RX_BUF_DISABLE 0 /*!< I2C master doesn't need buffer */
#define DELAY_TIME_BETWEEN_ITEMS_MS                                            \
  60 * 1000 /*!< delay time between different test items */

#define WRITE_BIT 0       /*!< I2C master write */
#define READ_BIT 1        /*!< I2C master read */
#define ACK_CHECK_EN 0x1  /*!< I2C master will check ack from slave*/
#define ACK_CHECK_DIS 0x0 /*!< I2C master will not check ack from slave */
#define ACK_VAL 0x0       /*!< I2C ack value */
#define NACK_VAL 0x1      /*!< I2C nack value */
/**
 * @brief test code to operate
 *
 * 1. set operation mode(e.g One time L-resolution mode)
 * _________________________________________________________________
 * | start | slave_addr + wr_bit + ack | write 1 byte + ack  | stop |
 * --------|---------------------------|---------------------|------|
 * 2. wait more than 24 ms
 * 3. read data
 * ______________________________________________________________________________________
 * | start | slave_addr + rd_bit + ack | read 1 byte + ack  | read 1 byte + nack
 * | stop |
 * --------|---------------------------|--------------------|--------------------|------|
 */

/**************************************************************************/
/*!
    @brief  Sends a single command byte over I2C
*/
/**************************************************************************/
static esp_err_t wireWriteRegister(INA219_instance_t *instance, uint8_t reg,
                                   uint16_t value) {
  uint8_t values[3];
  values[0] = reg;
  values[1] = (value >> 8) & 0xFF;
  values[2] = (value & 0xFF);
  esp_err_t ret = instance->I2C.i2c_send(instance->address, values, 3,
                                         instance->I2C.device);
  return ret;
}

/**************************************************************************/
/*!
    @brief  Reads a 16 bit values over I2C
*/
/**************************************************************************/
static esp_err_t wireReadRegister(INA219_instance_t *instance, uint8_t reg,
                                  uint16_t *value) {
  uint8_t values[2] = {0, 0};
  esp_err_t ret = instance->I2C.i2c_send_receive(
      instance->address, &reg, 1, values, 2, instance->I2C.device);
  *value = (uint16_t)(values[0] << 8) | (uint16_t)values[1];
  return ret;
}

/**************************************************************************/
/*!
    @brief  Configures to INA219 to be able to measure up to 32V and 2A
            of current.  Each unit of current corresponds to 100uA, and
            each unit of power corresponds to 2mW. Counter overflow
            occurs at 3.2A.

    @note   These calculations assume a 0.1 ohm resistor is present
*/
/**************************************************************************/
esp_err_t INA219_setCalibration_32V_2A(INA219_instance_t *instance) {
  // By default we use a pretty huge range for the input voltage,
  // which probably isn't the most appropriate choice for system
  // that don't use a lot of power.  But all of the calculations
  // are shown below if you want to change the settings.  You will
  // also need to change any relevant register settings, such as
  // setting the VBUS_MAX to 16V instead of 32V, etc.

  // VBUS_MAX = 32V             (Assumes 32V, can also be set to 16V)
  // VSHUNT_MAX = 0.32          (Assumes Gain 8, 320mV, can also be 0.16, 0.08,
  // 0.04) RSHUNT = 0.1               (Resistor value in ohms)

  // 1. Determine max possible current
  // MaxPossible_I = VSHUNT_MAX / RSHUNT
  // MaxPossible_I = 3.2A

  // 2. Determine max expected current
  // MaxExpected_I = 2.0A

  // 3. Calculate possible range of LSBs (Min = 15-bit, Max = 12-bit)
  // MinimumLSB = MaxExpected_I/32767
  // MinimumLSB = 0.000061              (61uA per bit)
  // MaximumLSB = MaxExpected_I/4096
  // MaximumLSB = 0,000488              (488uA per bit)

  // 4. Choose an LSB between the min and max values
  //    (Preferrably a roundish number close to MinLSB)
  // CurrentLSB = 0.0001 (100uA per bit)

  // 5. Compute the calibration register
  // Cal = trunc (0.04096 / (Current_LSB * RSHUNT))
  // Cal = 4096 (0x1000)

  instance->ina219_calValue = 4096;

  // 6. Calculate the power LSB
  // PowerLSB = 20 * CurrentLSB
  // PowerLSB = 0.002 (2mW per bit)

  // 7. Compute the maximum current and shunt voltage values before overflow
  //
  // Max_Current = Current_LSB * 32767
  // Max_Current = 3.2767A before overflow
  //
  // If Max_Current > Max_Possible_I then
  //    Max_Current_Before_Overflow = MaxPossible_I
  // Else
  //    Max_Current_Before_Overflow = Max_Current
  // End If
  //
  // Max_ShuntVoltage = Max_Current_Before_Overflow * RSHUNT
  // Max_ShuntVoltage = 0.32V
  //
  // If Max_ShuntVoltage >= VSHUNT_MAX
  //    Max_ShuntVoltage_Before_Overflow = VSHUNT_MAX
  // Else
  //    Max_ShuntVoltage_Before_Overflow = Max_ShuntVoltage
  // End If

  // 8. Compute the Maximum Power
  // MaximumPower = Max_Current_Before_Overflow * VBUS_MAX
  // MaximumPower = 3.2 * 32V
  // MaximumPower = 102.4W

  // Set multipliers to convert raw current/power values
  instance->ina219_currentDivider_mA =
      10; // Current LSB = 100uA per bit (1000/100 = 10)
  instance->ina219_powerDivider_mW = 2; // Power LSB = 1mW per bit (2/1)

  // Set Calibration register to 'Cal' calculated above
  esp_err_t ret = wireWriteRegister(instance, INA219_REG_CALIBRATION,
                                    instance->ina219_calValue);
  if (ret != ESP_OK) {
    return ret;
  }

  // Set Config register to take into account the settings above
  uint16_t config = INA219_CONFIG_BVOLTAGERANGE_32V |
                    INA219_CONFIG_GAIN_8_320MV | INA219_CONFIG_BADCRES_12BIT |
                    INA219_CONFIG_SADCRES_12BIT_1S_532US |
                    INA219_CONFIG_MODE_SANDBVOLT_CONTINUOUS;
  ret = wireWriteRegister(instance, INA219_REG_CONFIG, config);
  return ret;
}

/**************************************************************************/
/*!
    @brief  Configures to INA219 to be able to measure up to 32V and 1A
            of current.  Each unit of current corresponds to 40uA, and each
            unit of power corresponds to 800�W. Counter overflow occurs at
            1.3A.

    @note   These calculations assume a 0.1 ohm resistor is present
*/
/**************************************************************************/
esp_err_t INA219_setCalibration_32V_1A(INA219_instance_t *instance) {
  // By default we use a pretty huge range for the input voltage,
  // which probably isn't the most appropriate choice for system
  // that don't use a lot of power.  But all of the calculations
  // are shown below if you want to change the settings.  You will
  // also need to change any relevant register settings, such as
  // setting the VBUS_MAX to 16V instead of 32V, etc.

  // VBUS_MAX = 32V		(Assumes 32V, can also be set to 16V)
  // VSHUNT_MAX = 0.32	(Assumes Gain 8, 320mV, can also be 0.16, 0.08, 0.04)
  // RSHUNT = 0.1			(Resistor value in ohms)

  // 1. Determine max possible current
  // MaxPossible_I = VSHUNT_MAX / RSHUNT
  // MaxPossible_I = 3.2A

  // 2. Determine max expected current
  // MaxExpected_I = 1.0A

  // 3. Calculate possible range of LSBs (Min = 15-bit, Max = 12-bit)
  // MinimumLSB = MaxExpected_I/32767
  // MinimumLSB = 0.0000305             (30.5�A per bit)
  // MaximumLSB = MaxExpected_I/4096
  // MaximumLSB = 0.000244              (244�A per bit)

  // 4. Choose an LSB between the min and max values
  //    (Preferrably a roundish number close to MinLSB)
  // CurrentLSB = 0.0000400 (40�A per bit)

  // 5. Compute the calibration register
  // Cal = trunc (0.04096 / (Current_LSB * RSHUNT))
  // Cal = 10240 (0x2800)

  instance->ina219_calValue = 10240;

  // 6. Calculate the power LSB
  // PowerLSB = 20 * CurrentLSB
  // PowerLSB = 0.0008 (800�W per bit)

  // 7. Compute the maximum current and shunt voltage values before overflow
  //
  // Max_Current = Current_LSB * 32767
  // Max_Current = 1.31068A before overflow
  //
  // If Max_Current > Max_Possible_I then
  //    Max_Current_Before_Overflow = MaxPossible_I
  // Else
  //    Max_Current_Before_Overflow = Max_Current
  // End If
  //
  // ... In this case, we're good though since Max_Current is less than
  // MaxPossible_I
  //
  // Max_ShuntVoltage = Max_Current_Before_Overflow * RSHUNT
  // Max_ShuntVoltage = 0.131068V
  //
  // If Max_ShuntVoltage >= VSHUNT_MAX
  //    Max_ShuntVoltage_Before_Overflow = VSHUNT_MAX
  // Else
  //    Max_ShuntVoltage_Before_Overflow = Max_ShuntVoltage
  // End If

  // 8. Compute the Maximum Power
  // MaximumPower = Max_Current_Before_Overflow * VBUS_MAX
  // MaximumPower = 1.31068 * 32V
  // MaximumPower = 41.94176W

  // Set multipliers to convert raw current/power values
  instance->ina219_currentDivider_mA =
      25; // Current LSB = 40uA per bit (1000/40 = 25)
  instance->ina219_powerDivider_mW = 1; // Power LSB = 800�W per bit

  // Set Calibration register to 'Cal' calculated above
  esp_err_t ret = wireWriteRegister(instance, INA219_REG_CALIBRATION,
                                    instance->ina219_calValue);
  if (ret != ESP_OK) {
    return ret;
  }

  // Set Config register to take into account the settings above
  uint16_t config = INA219_CONFIG_BVOLTAGERANGE_32V |
                    INA219_CONFIG_GAIN_8_320MV | INA219_CONFIG_BADCRES_12BIT |
                    INA219_CONFIG_SADCRES_12BIT_1S_532US |
                    INA219_CONFIG_MODE_SANDBVOLT_CONTINUOUS;
  ret = wireWriteRegister(instance, INA219_REG_CONFIG, config);
  return ret;
}
int INA219_setCalibration_16V_1A_100mOhm(INA219_instance_t *instance) {
  // By default we use a pretty huge range for the input voltage,
  // which probably isn't the most appropriate choice for system
  // that don't use a lot of power.  But all of the calculations
  // are shown below if you want to change the settings.  You will
  // also need to change any relevant register settings, such as
  // setting the VBUS_MAX to 16V instead of 32V, etc.

  // VBUS_MAX = 16V
  // VSHUNT_MAX = 0.16	(Assumes Gain 4, 160mV)
  // RSHUNT = 0.1			(Resistor value in ohms)

  // 1. Determine max possible current
  // MaxPossible_I = VSHUNT_MAX / RSHUNT
  // MaxPossible_I = 1.6A

  // 2. Determine max expected current
  // MaxExpected_I = 1.6A

  // 3. Calculate possible range of LSBs (Min = 15-bit, Max = 12-bit)
  // MinimumLSB = MaxExpected_I/32767
  // MinimumLSB = 0.0000488             (48.8uA per bit)
  // MaximumLSB = MaxExpected_I/4096
  // MaximumLSB = 0.0003907             (390uA per bit)

  // 4. Choose an LSB between the min and max values
  //    (Preferrably a roundish number close to MinLSB)
  // CurrentLSB = 0.0000500 (50uA per bit)

  // 5. Compute the calibration register
  // Cal = trunc (0.0000500 / (Current_LSB * RSHUNT))
  // Cal = 10240 (0x2800)

  instance->ina219_calValue = 1638;

  // 6. Calculate the power LSB
  // PowerLSB = 20 * CurrentLSB
  // PowerLSB = 0.0001 (1000uW per bit)

  // 7. Compute the maximum current and shunt voltage values before overflow
  //
  // Max_Current = Current_LSB * 32767
  // Max_Current = 1.31068A before overflow
  //
  // If Max_Current > Max_Possible_I then
  //    Max_Current_Before_Overflow = MaxPossible_I
  // Else
  //    Max_Current_Before_Overflow = Max_Current
  // End If
  //
  // ... In this case, we're good though since Max_Current is less than
  // MaxPossible_I
  //
  // Max_ShuntVoltage = Max_Current_Before_Overflow * RSHUNT
  // Max_ShuntVoltage = 0.131068V
  //
  // If Max_ShuntVoltage >= VSHUNT_MAX
  //    Max_ShuntVoltage_Before_Overflow = VSHUNT_MAX
  // Else
  //    Max_ShuntVoltage_Before_Overflow = Max_ShuntVoltage
  // End If

  // 8. Compute the Maximum Power
  // MaximumPower = Max_Current_Before_Overflow * VBUS_MAX
  // MaximumPower = 1.31068 * 16V
  // MaximumPower = 41.94176W

  // Set multipliers to convert raw current/power values
  instance->ina219_currentDivider_mA =
      4; // Current LSB = 50uA per bit (1000/300 = 25)
  instance->ina219_powerDivider_mW = 5; // Power LSB = 5W per bit

  // Set Calibration register to 'Cal' calculated above
  int ret = wireWriteRegister(instance, INA219_REG_CALIBRATION,
                              instance->ina219_calValue);
  if (ret != ESP_OK)
    return ret;

  // Set Config register to take into account the settings above
  uint16_t config = INA219_CONFIG_BVOLTAGERANGE_16V |
                    INA219_CONFIG_GAIN_8_320MV | INA219_CONFIG_BADCRES_12BIT |
                    INA219_CONFIG_SADCRES_12BIT_1S_532US |
                    INA219_CONFIG_MODE_SANDBVOLT_CONTINUOUS;
  ret = wireWriteRegister(instance, INA219_REG_CONFIG, config);
  if (ret != ESP_OK)
    return ret;
  uint16_t configRead = 0;
  ret = wireReadRegister(instance, INA219_REG_CONFIG, &configRead);
  if (ret != ESP_OK)
    return ret;
  if (config != configRead) {
    return -1;
  } else {
    return 0;
  }
}
int INA219_setCalibration_16V_400mA(INA219_instance_t *instance) {

  // Calibration which uses the highest precision for
  // current measurement (0.1mA), at the expense of
  // only supporting 16V at 400mA max.

  // VBUS_MAX = 16V
  // VSHUNT_MAX = 0.04          (Assumes Gain 1, 40mV)
  // RSHUNT = 0.1               (Resistor value in ohms)

  // 1. Determine max possible current
  // MaxPossible_I = VSHUNT_MAX / RSHUNT
  // MaxPossible_I = 0.4A

  // 2. Determine max expected current
  // MaxExpected_I = 0.4A

  // 3. Calculate possible range of LSBs (Min = 15-bit, Max = 12-bit)
  // MinimumLSB = MaxExpected_I/32767
  // MinimumLSB = 0.0000122              (12uA per bit)
  // MaximumLSB = MaxExpected_I/4096
  // MaximumLSB = 0.0000977              (98uA per bit)

  // 4. Choose an LSB between the min and max values
  //    (Preferably a roundish number close to MinLSB)
  // CurrentLSB = 0.00005 (50uA per bit)

  // 5. Compute the calibration register
  // Cal = trunc (0.04096 / (Current_LSB * RSHUNT))
  // Cal = 8192 (0x2000)

  instance->ina219_calValue = 8192;

  // 6. Calculate the power LSB
  // PowerLSB = 20 * CurrentLSB
  // PowerLSB = 0.001 (1mW per bit)

  // 7. Compute the maximum current and shunt voltage values before overflow
  //
  // Max_Current = Current_LSB * 32767
  // Max_Current = 1.63835A before overflow
  //
  // If Max_Current > Max_Possible_I then
  //    Max_Current_Before_Overflow = MaxPossible_I
  // Else
  //    Max_Current_Before_Overflow = Max_Current
  // End If
  //
  // Max_Current_Before_Overflow = MaxPossible_I
  // Max_Current_Before_Overflow = 0.4
  //
  // Max_ShuntVoltage = Max_Current_Before_Overflow * RSHUNT
  // Max_ShuntVoltage = 0.04V
  //
  // If Max_ShuntVoltage >= VSHUNT_MAX
  //    Max_ShuntVoltage_Before_Overflow = VSHUNT_MAX
  // Else
  //    Max_ShuntVoltage_Before_Overflow = Max_ShuntVoltage
  // End If
  //
  // Max_ShuntVoltage_Before_Overflow = VSHUNT_MAX
  // Max_ShuntVoltage_Before_Overflow = 0.04V

  // 8. Compute the Maximum Power
  // MaximumPower = Max_Current_Before_Overflow * VBUS_MAX
  // MaximumPower = 0.4 * 16V
  // MaximumPower = 6.4W

  // Set multipliers to convert raw current/power values
  instance->ina219_currentDivider_mA =
      20; // Current LSB = 50uA per bit (1000/50 = 20)
  instance->ina219_powerDivider_mW = 1; // Power LSB = 1mW per bit

  // Set Calibration register to 'Cal' calculated above
  int ret = wireWriteRegister(instance, INA219_REG_CALIBRATION,
                              instance->ina219_calValue);
  if (ret != ESP_OK)
    return ret;
  // Set Config register to take into account the settings above
  uint16_t config = INA219_CONFIG_BVOLTAGERANGE_16V |
                    INA219_CONFIG_GAIN_1_40MV | INA219_CONFIG_BADCRES_12BIT |
                    INA219_CONFIG_SADCRES_12BIT_1S_532US |
                    INA219_CONFIG_MODE_SANDBVOLT_CONTINUOUS;
  ret = wireWriteRegister(instance, INA219_REG_CONFIG, config);
  if (ret != ESP_OK)
    return ret;
  uint16_t configRead = 0;
  ret = wireReadRegister(instance, INA219_REG_CONFIG, &configRead);
  if (ret != ESP_OK)
    return ret;
  if (config != configRead) {
    return -1;
  } else {
    return 0;
  }
}
esp_err_t INA219_setCalibration_16V_40mA_1ohm(INA219_instance_t *instance) {

  // Calibration which uses the highest precision for
  // current measurement (0.1mA), at the expense of
  // only supporting 16V at 400mA max.

  // VBUS_MAX = 16V
  // VSHUNT_MAX = 0.04          (Assumes Gain 1, 40mV)
  // RSHUNT = 1                 (Resistor value in ohms)

  // 1. Determine max possible current
  // MaxPossible_I = VSHUNT_MAX / RSHUNT
  // MaxPossible_I = 0.04A

  // 2. Determine max expected current
  // MaxExpected_I = 0.04A

  // 3. Calculate possible range of LSBs (Min = 15-bit, Max = 12-bit)
  // MinimumLSB = MaxExpected_I/32767
  // MinimumLSB = 0.00000122              (1.2uA per bit)
  // MaximumLSB = MaxExpected_I/4096
  // MaximumLSB = 0.00000977              (9.8uA per bit)

  // 4. Choose an LSB between the min and max values
  //    (Preferrably a roundish number close to MinLSB)
  // Current_LSB = 0.000005 (5uA per bit)

  // 5. Compute the calibration register
  // Cal = trunc (0.04096 / (Current_LSB * RSHUNT))
  // Cal = 8192 (0x2000)

  instance->ina219_calValue = 8192;

  // 6. Calculate the power LSB
  // PowerLSB = 20 * Current_LSB
  // PowerLSB = 0.0001 (0.1mW per bit)

  // 7. Compute the maximum current and shunt voltage values before overflow
  //
  // Max_Current = Current_LSB * 32767
  // Max_Current = 0.163835A before overflow
  //
  // If Max_Current > Max_Possible_I then
  //    Max_Current_Before_Overflow = MaxPossible_I
  // Else
  //    Max_Current_Before_Overflow = Max_Current
  // End If
  //
  // Max_Current_Before_Overflow = MaxPossible_I
  // Max_Current_Before_Overflow = 0.4
  //
  // Max_ShuntVoltage = Max_Current_Before_Overflow * RSHUNT
  // Max_ShuntVoltage = 0.04V
  //
  // If Max_ShuntVoltage >= VSHUNT_MAX
  //    Max_ShuntVoltage_Before_Overflow = VSHUNT_MAX
  // Else
  //    Max_ShuntVoltage_Before_Overflow = Max_ShuntVoltage
  // End If
  //
  // Max_ShuntVoltage_Before_Overflow = VSHUNT_MAX
  // Max_ShuntVoltage_Before_Overflow = 0.04V

  // 8. Compute the Maximum Power
  // MaximumPower = Max_Current_Before_Overflow * VBUS_MAX
  // MaximumPower = 0.4 * 16V
  // MaximumPower = 6.4W

  // Set multipliers to convert raw current/power values
  instance->ina219_currentDivider_mA =
      200; // Current LSB = 5uA per bit (1000/5 = 200)
  instance->ina219_powerDivider_mW = 0.1f; // Power LSB = 0.1mW per bit

  // Set Calibration register to 'Cal' calculated above
  esp_err_t ret = wireWriteRegister(instance, INA219_REG_CALIBRATION,
                                    instance->ina219_calValue);
  if (ret != ESP_OK)
    return ret;

  // Set Config register to take into account the settings above
  uint16_t config = INA219_CONFIG_BVOLTAGERANGE_16V |
                    INA219_CONFIG_GAIN_1_40MV | INA219_CONFIG_BADCRES_12BIT |
                    INA219_CONFIG_SADCRES_12BIT_1S_532US |
                    INA219_CONFIG_MODE_SANDBVOLT_CONTINUOUS;
  ret = wireWriteRegister(instance, INA219_REG_CONFIG, config);
  if (ret != ESP_OK)
    return ret;
  ret = wireReadRegister(instance, INA219_REG_CONFIG, &config);
  return ret;
}
esp_err_t INA219_setCalibration_16V_40mA_4mohm(INA219_instance_t *instance) {

  // Calibration which uses the highest precision for
  // current measurement (0.1mA), at the expense of
  // only supporting 16V at 400mA max.

  // VBUS_MAX = 16V
  // VSHUNT_MAX = 0.04          (Assumes Gain 1, 40mV)
  // RSHUNT = .4                (Resistor value in ohms)

  // 1. Determine max possible current
  // MaxPossible_I = VSHUNT_MAX / RSHUNT
  // MaxPossible_I = 0.1A

  // 2. Determine max expected current
  // MaxExpected_I = 0.04A

  // 3. Calculate possible range of LSBs (Min = 15-bit, Max = 12-bit)
  // MinimumLSB = MaxExpected_I/32767
  // MinimumLSB = 0.00000122              (1.2uA per bit)
  // MaximumLSB = MaxExpected_I/4096
  // MaximumLSB = 0.00000977              (9.8uA per bit)

  // 4. Choose an LSB between the min and max values
  //    (Preferrably a roundish number close to MinLSB)
  // Current_LSB = 0.000002 (2uA per bit)

  // 5. Compute the calibration register
  // Cal = trunc (0.04096 / (Current_LSB * RSHUNT))
  // Cal = 51200 (0x2000)

  instance->ina219_calValue = 51200;

  // 6. Calculate the power LSB
  // PowerLSB = 20 * Current_LSB
  // PowerLSB = 0.00004 (0.040mW per bit)

  // 7. Compute the maximum current and shunt voltage values before overflow
  //
  // Max_Current = Current_LSB * 32767
  // Max_Current = 0.163835A before overflow
  //
  // If Max_Current > Max_Possible_I then
  //    Max_Current_Before_Overflow = MaxPossible_I
  // Else
  //    Max_Current_Before_Overflow = Max_Current
  // End If
  //
  // Max_Current_Before_Overflow = MaxPossible_I
  // Max_Current_Before_Overflow = 0.4
  //
  // Max_ShuntVoltage = Max_Current_Before_Overflow * RSHUNT
  // Max_ShuntVoltage = 0.04V
  //
  // If Max_ShuntVoltage >= VSHUNT_MAX
  //    Max_ShuntVoltage_Before_Overflow = VSHUNT_MAX
  // Else
  //    Max_ShuntVoltage_Before_Overflow = Max_ShuntVoltage
  // End If
  //
  // Max_ShuntVoltage_Before_Overflow = VSHUNT_MAX
  // Max_ShuntVoltage_Before_Overflow = 0.04V

  // 8. Compute the Maximum Power
  // MaximumPower = Max_Current_Before_Overflow * VBUS_MAX
  // MaximumPower = 0.4 * 16V
  // MaximumPower = 6.4W

  // Set multipliers to convert raw current/power values
  instance->ina219_currentDivider_mA =
      500; // Current LSB = 2uA per bit (1000/2 = 500)
  instance->ina219_powerDivider_mW = 0.040f; // Power LSB = 0.040mW per bit

  // Set Calibration register to 'Cal' calculated above
  esp_err_t ret = wireWriteRegister(instance, INA219_REG_CALIBRATION,
                                    instance->ina219_calValue);
  if (ret != ESP_OK) {
    return ret;
  }

  // Set Config register to take into account the settings above
  uint16_t config = INA219_CONFIG_BVOLTAGERANGE_16V |
                    INA219_CONFIG_GAIN_1_40MV | INA219_CONFIG_BADCRES_12BIT |
                    INA219_CONFIG_SADCRES_12BIT_1S_532US |
                    INA219_CONFIG_MODE_SANDBVOLT_CONTINUOUS;
  ret = wireWriteRegister(instance, INA219_REG_CONFIG, config);
  if (ret != ESP_OK) {
    return ret;
  }
  ret = wireReadRegister(instance, INA219_REG_CONFIG, &config);
  return ret;
}

esp_err_t INA219_setCalibration_16V_320mA_1ohm(INA219_instance_t *instance) {

  // Calibration which uses the highest precision for
  // current measurement (0.1mA), at the expense of
  // only supporting 16V at 400mA max.

  // VBUS_MAX = 16V
  // VSHUNT_MAX = 0.320          (Assumes Gain 1, 320mV)
  // RSHUNT = 1                 (Resistor value in ohms)

  // 1. Determine max possible current
  // MaxPossible_I = VSHUNT_MAX / RSHUNT
  // MaxPossible_I = 0.320A

  // 2. Determine max expected current
  // MaxExpected_I = 0.320A

  // 3. Calculate possible range of LSBs (Min = 15-bit, Max = 12-bit)
  // MinimumLSB = MaxExpected_I/32767
  // MinimumLSB = 0.0000122              (12uA per bit)
  // MaximumLSB = MaxExpected_I/4096
  // MaximumLSB = 0.0000977              (98uA per bit)

  // 4. Choose an LSB between the min and max values
  //    (Preferrably a roundish number close to MinLSB)
  // CurrentLSB = 0.000005 (5uA per bit)

  // 5. Compute the calibration register
  // Cal = trunc (0.04096 / (Current_LSB * RSHUNT))
  // Cal = 8192 (0x2000)

  instance->ina219_calValue = 8192;

  // 6. Calculate the power LSB
  // PowerLSB = 20 * CurrentLSB
  // PowerLSB = 0.0001 (0.1mW per bit)

  // 7. Compute the maximum current and shunt voltage values before overflow
  //
  // Max_Current = Current_LSB * 32767
  // Max_Current = 0.163835A before overflow
  //
  // If Max_Current > Max_Possible_I then
  //    Max_Current_Before_Overflow = MaxPossible_I
  // Else
  //    Max_Current_Before_Overflow = Max_Current
  // End If
  //
  // Max_Current_Before_Overflow = MaxPossible_I
  // Max_Current_Before_Overflow = 0.4
  //
  // Max_ShuntVoltage = Max_Current_Before_Overflow * RSHUNT
  // Max_ShuntVoltage = 0.04V
  //
  // If Max_ShuntVoltage >= VSHUNT_MAX
  //    Max_ShuntVoltage_Before_Overflow = VSHUNT_MAX
  // Else
  //    Max_ShuntVoltage_Before_Overflow = Max_ShuntVoltage
  // End If
  //
  // Max_ShuntVoltage_Before_Overflow = VSHUNT_MAX
  // Max_ShuntVoltage_Before_Overflow = 0.04V

  // 8. Compute the Maximum Power
  // MaximumPower = Max_Current_Before_Overflow * VBUS_MAX
  // MaximumPower = 0.4 * 16V
  // MaximumPower = 6.4W

  // Set multipliers to convert raw current/power values

  instance->ina219_currentDivider_mA =
      200; // Current LSB = 5uA per bit (1000/5 = 200)
  instance->ina219_powerDivider_mW = .1f; // Power LSB = .1mW per bit

  // Set Calibration register to 'Cal' calculated above
  esp_err_t ret = wireWriteRegister(instance, INA219_REG_CALIBRATION,
                                    instance->ina219_calValue);
  if (ret != ESP_OK) {
    return ret;
  }
  // Set Config register to take into account the settings above
  uint16_t config = INA219_CONFIG_BVOLTAGERANGE_16V |
                    INA219_CONFIG_GAIN_8_320MV | INA219_CONFIG_BADCRES_12BIT |
                    INA219_CONFIG_SADCRES_12BIT_1S_532US |
                    INA219_CONFIG_MODE_SANDBVOLT_CONTINUOUS;
  ret = wireWriteRegister(instance, INA219_REG_CONFIG, config);
  if (ret != ESP_OK) {
    return ret;
  }
  ret = wireReadRegister(instance, INA219_REG_CONFIG, &config);
  return ret;
}

/**************************************************************************/
/*!
    @brief  Gets the raw bus voltage (16-bit signed integer, so +-32767)
*/
/**************************************************************************/
esp_err_t INA219_getBusVoltage_raw(INA219_instance_t *instance,
                                   int16_t *value) {
  uint16_t valueu;
  esp_err_t ret = wireReadRegister(instance, INA219_REG_BUSVOLTAGE, &valueu);

  // Shift to the right 3 to drop CNVR and OVF and multiply by LSB
  *value = (int16_t)((valueu >> 3) * 4);
  return ret;
}

/**************************************************************************/
/*!
    @brief  Gets the raw shunt voltage (16-bit signed integer, so +-32767)
*/
/**************************************************************************/
esp_err_t INA219_getShuntVoltage_raw(INA219_instance_t *instance,
                                     int16_t *value) {
  esp_err_t ret =
      wireReadRegister(instance, INA219_REG_SHUNTVOLTAGE, (uint16_t *)value);
  return ret;
}

/**************************************************************************/
/*!
    @brief  Gets the raw current value (16-bit signed integer, so +-32767)
*/
/**************************************************************************/
esp_err_t INA219_getCurrent_raw(INA219_instance_t *instance, int16_t *current) {
  uint16_t value;

  // Sometimes a sharp load will reset the INA219, which will
  // reset the cal register, meaning CURRENT and POWER will
  // not be available ... avoid this by always setting a cal
  // value even if it's an unfortunate extra step
  esp_err_t ret = wireWriteRegister(instance, INA219_REG_CALIBRATION,
                                    instance->ina219_calValue);
  if (ret != ESP_OK)
    return ret;

  // Now we can safely read the CURRENT register!
  ret = wireReadRegister(instance, INA219_REG_CURRENT, &value);

  if (value == 0x8000)
    value = 0;
  *current = value;
  return ret;
}
/**************************************************************************/
/*!
    @brief  Gets the raw shunt voltage (16-bit signed integer, so +-32767)
*/
/**************************************************************************/
esp_err_t INA219_getPower_raw(INA219_instance_t *instance, float *power) {
  esp_err_t ret =
      wireReadRegister(instance, INA219_REG_POWER, (uint16_t *)power);
  return ret;
}
/**************************************************************************/
/*!
    @brief  Gets the shunt voltage in mV (so +-327mV)
*/
/**************************************************************************/
esp_err_t INA219_getShuntVoltage_mV(INA219_instance_t *instance, float *volt) {
  int16_t value;
  esp_err_t ret = INA219_getShuntVoltage_raw(instance, &value);
  *volt = (float)value * 0.01;

  return ret;
}

/**************************************************************************/
/*!
    @brief  Gets the shunt voltage in volts
*/
/**************************************************************************/
esp_err_t INA219_getBusVoltage_V(INA219_instance_t *instance, float *volt) {
  int16_t value;
  esp_err_t ret = INA219_getBusVoltage_raw(instance, &value);
  *volt = (float)value * 0.001;
  return ret;
}

/**************************************************************************/
/*!
    @brief  Gets the current value in mA, taking into account the
            config settings and current LSB
*/
/**************************************************************************/
esp_err_t INA219_getCurrent_mA(INA219_instance_t *instance, float *current) {
  int16_t valueDec = 0;
  esp_err_t ret = INA219_getCurrent_raw(instance, &valueDec);
  *current = (float)valueDec / instance->ina219_currentDivider_mA;
  return ret;
}

/**************************************************************************/
/*!
    @brief  Gets the current value in mA, taking into account the
            config settings and current LSB
*/
/**************************************************************************/

esp_err_t INA219_getPower_mW(INA219_instance_t *instance, float *power) {
  float valueDec = 0;
  esp_err_t ret = INA219_getPower_raw(instance, &valueDec);
  if (ret != ESP_OK)
    return ret;
  valueDec *= instance->ina219_powerDivider_mW;
  return ret;
}

void INA219_setSleepMode(
    INA219_instance_t
        *instance) { /// todo: power states need to be implemented.
  uint16_t config = 0;
  wireReadRegister(instance, INA219_REG_CURRENT, &config);
  config = (config & ~INA219_CONFIG_MODE_MASK) | INA219_CONFIG_MODE_POWERDOWN;
  wireWriteRegister(instance, INA219_REG_CONFIG, config);
}

/**************************************************************************/
/*!
    @brief  Task to read data from sensor.
*/
/**************************************************************************/

esp_err_t INA219_set_Calibration(INA219_instance_t *instance) {
  switch (instance->calibration) {
  case CAL_32V_2A:
    return INA219_setCalibration_32V_2A(instance);
    break;
  case CAL_32V_1A:
    return INA219_setCalibration_32V_1A(instance);
    break;
  case CAL_16V_1A_100mOhm:
    return INA219_setCalibration_16V_1A_100mOhm(instance);
    break;
  case CAL_16V_400mA:
    return INA219_setCalibration_16V_400mA(instance);
    break;
  case CAL_16V_40mA_1ohm:
    return INA219_setCalibration_16V_40mA_1ohm(instance);
    break;
  case CAL_16V_40mA_4mohm:
    return INA219_setCalibration_16V_40mA_4mohm(instance);
    break;
  case CAL_16V_320mA_1ohm:
    return INA219_setCalibration_16V_320mA_1ohm(instance);
    break;
  default:
    ESP_LOGE(TAG, "Invalid calibration value");
    return ESP_ERR_INVALID_ARG;
  }
}
// User side implementation functions
esp_err_t INA219_device_available(INA219_instance_t *instance) {
  return instance->I2C.device_available(instance->address,
                                        instance->I2C.device);
}

void INA219_get_device_values(INA219_instance_t *instance) {
  ina219_data data = {0};
  esp_err_t err = ESP_FAIL;

  err = INA219_getShuntVoltage_mV(instance, &data.ShuntVoltage_mV);
  if (err == ESP_OK && data.ShuntVoltage_mV < MAX_SHUNT_VOLTAGE &&
      data.ShuntVoltage_mV > MIN_SHUNT_VOLTAGE) {
    instance->device_data.ShuntVoltage_mV = data.ShuntVoltage_mV;
  }

  err = INA219_getBusVoltage_V(instance, &data.Voltage_V);
  if (err == ESP_OK && data.Voltage_V < MAX_VOLTAGE &&
      data.Voltage_V > MIN_VOLTAGE) {
    instance->device_data.Voltage_V = data.Voltage_V;
  }

  err = INA219_getCurrent_mA(instance, &data.Current_mA);
  if (err == ESP_OK && data.Current_mA < MAX_CURRENT && data.Current_mA > 0) {
    instance->device_data.Current_mA = data.Current_mA;
  }

  err = INA219_getPower_mW(instance, &data.Power_mW);
  if (err == ESP_OK && data.Power_mW < MAX_POWER && data.Power_mW > 0) {
    instance->device_data.Power_mW = data.Power_mW;
  }
}

/**************************************************************************/
/*!
    @brief  Instantiates a new INA219 class
*/
/**************************************************************************/
esp_err_t INA219_Init(INA219_instance_t *instance) {
  esp_err_t err = ESP_FAIL;
  if (instance->address == 0 || instance->calibration == 0) {
    ESP_LOGE(TAG, "Invalid address or calibration value");
    return ESP_ERR_INVALID_ARG;
  }
  err = INA219_set_Calibration(instance);
  instance->device_available = INA219_device_available;
  instance->get_device_values = INA219_get_device_values;
  return err;
}