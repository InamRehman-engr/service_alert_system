# Description

The INA226 is a current Shunt and power monitor IC introduced by texas Instruments. it uses an I2C or SMBUSS interface, it uses a shunt resistor to detect current and also monitors supply voltage. it also has internal power calculation with very low offset. Programmable calibration, averaging and timing allows for a flexible usecase.

# Pros / Cons
The INA226 platform allows us to take the Voltage ,current and power calculation away from the main microcontroller with a high resolution of 16-bit which most cheap microcontroller don't have. This allows us to use ROM, RAM and CPU cycles on more important tasks. This has very low offset and internal calibration as well as taking up 2 pins for I2c lines with programmable addresses. All this comes at the cost of additional hardware cost of INA219 and its necessary components.

# Interface
INA226 is capable of using I2C and SMBus-compatible interfaces for communication with a maximum speed of 2.94 MHz. I2c lines don't have internal pullups and need to be either installed or done through the host microcontroller. Slave address can be configured using the A1 & A2 pins.(refer to [datasheet](https://www.ti.com/lit/ds/symlink/ina226.pdf?ts=1707462545250&ref_url=https%253A%252F%252Fwww.google.com%252F) for more information)

# Example
This is the basic example that would be used for the sensor be sure to enable I2C and INA226 configs config defines are given below. for more information on available functionality please reference the API section.
```c
CONFIG_ENABLE_INA226
CONFIG_HARDWARE_I2C
```

Initialization of the sensor
----------------------------

```c
  // setting up the INA226
  i2c_device_t device;
  device.port = I2C_MASTER_NUM;
  INA226_instance_t INA226;
  INA226.I2C.device = &device;
  i2c_init(I2C_MODE_MASTER, I2C_MASTER_SDA_IO, I2C_MASTER_SCL_IO,
            GPIO_PULLUP_DISABLE, GPIO_PULLUP_DISABLE, I2C_MASTER_FREQ_HZ,
            &INA226.I2C);
  
  INA226.address = 0x40;
  INA226._maxCurrent = 10.0;
  INA226._shunt = 0.1;
  INA226_init(&INA226);

```
this configuration is already configured for 0.1Ω of shunt resister ans 10A of current.

Data collection
---------------

```c
  // read current from the INA226
  INA226.get_device_values(&INA226);
  ESP_LOGI("INA226", "Voltage: %fV", INA226.device_data.Voltage_V);
  ESP_LOGI("INA226", "Current: %fA", INA226.device_data.Current_A);
  ESP_LOGI("INA226", "Power:   %fW", INA226.device_data.Power_W);
  ESP_LOGI("INA226", "Shunt:   %fmV\n", INA226.device_data.Shunt_Voltage_mV);
```

# Range & Limitations
The table below shows some ranges and characteristics, please refer to the [datasheet](https://www.ti.com/lit/ds/symlink/ina226.pdf?ts=1707462545250&ref_url=https%253A%252F%252Fwww.google.com%252F) for more information.

ELECTRICAL CHARACTERISTICS
--------------------------

| Name                        | Range & Description                                       | Unit |
|-----------------------------|-----------------------------------------------------------|------|
| Measurement Error           | ±0.02 -> ±0.1 depending on temperature                    | %    |
| Current Sense Voltage Range | ±81.92 depending on PGA set(1 to 8)                       | mV   |
| Bus Voltage Range           | 0 -> 36                                                   | V    |
| ADC Resolution              | 16                                                        | bits |
| Voltage supply Range        | 2.7 -> 5.5                                                | V    |
| I2C SCL Operating Frequency | (Fast mode) 0.001 -> 0.4, (HIGH-SPEED MODE) 0.001 -> 2.94 | MHz  |