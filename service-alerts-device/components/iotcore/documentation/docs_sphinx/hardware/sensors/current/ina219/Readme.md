# Description

INA219 is a shunt Current Sensor module introduced by Texas instruments. It is a Zero-Drift, Bidirectional, Power Monitor module that monitors shunt voltage, Bus voltage, current, and power. With programmable conversion times and filtering. A programmable calibration value, combined with an internal multiplier, enables direct readouts in amperes. An additional multiplying register calculates power in watts.

Pin Outs
--------
![Alt text](./pin_out.png)

Block Diagram
-------------
![Alt text](./block_diagram.png)

The image above shows the block diagram and the typical circuit to be used with the device, note that the A0 and A1 lines are to be permanently strapped to an appropriate line as per the address requirements please refer to the Table 1. INA219 Address Pins and Slave Addresses of the [datasheet](https://www.ti.com/lit/ds/symlink/ina219.pdf?ts=1707265713398&ref_url=https%253A%252F%252Fwww.ti.com%252Fproduct%252FINA219%253Futm_source%253Dgoogle%2526utm_medium%253Dcpc%2526utm_campaign%253Dasc-amps-null-44700045336317092_prodfolderdynamic-cpc-pf-google-wwe_int%2526utm_content%253Dprodfolddynamic%2526ds_k%253DDYNAMIC%2BSEARCH%2BADS%2526DCM%253Dyes%2526gad_source%253D1%2526gclid%253DEAIaIQobChMIgL2D6fuXhAMV5RatBh2YrwioEAAYASAAEgK84fD_BwE%2526gclsrc%253Daw.ds).

Register Block Diagram
----------------------
![Alt text](./register_block_diagram.png)

# Pros / Cons
The INA219 platform allows us to take the Voltage ,current and power calculation away from the main microcontroller. This allows us to use ROM, RAM and CPU cycles on more important tasks. This has very low offset and internal calibration as well as taking up 2 pins for I2c lines with programmable addresses. All this comes at the cost of additional hardware cost of INA219 and its necessary components.

# Interface
INA219 is capable of using I2C and SMBus-compatible interfaces for communication. I2c lines don't have internal pullups and need to be either installed or done through the host microcontroller. Slave address can be configured using the A1 & A2 pins.(refer to [datasheet](https://www.ti.com/lit/ds/symlink/ina219.pdf?ts=1707265713398&ref_url=https%253A%252F%252Fwww.ti.com%252Fproduct%252FINA219%253Futm_source%253Dgoogle%2526utm_medium%253Dcpc%2526utm_campaign%253Dasc-amps-null-44700045336317092_prodfolderdynamic-cpc-pf-google-wwe_int%2526utm_content%253Dprodfolddynamic%2526ds_k%253DDYNAMIC%2BSEARCH%2BADS%2526DCM%253Dyes%2526gad_source%253D1%2526gclid%253DEAIaIQobChMIgL2D6fuXhAMV5RatBh2YrwioEAAYASAAEgK84fD_BwE%2526gclsrc%253Daw.ds) for more information)

# Example
This is the basic example that would be used for the sensor be sure to enable I2C and INA226 configs config defines are given below. for more information on available functionality please reference the API section.
```c
CONFIG_ENABLE_INA219
CONFIG_HARDWARE_I2C
```

Initialization of the sensor
----------------------------

```c
  // setting up the INA219
  i2c_device_t device;
  device.port = I2C_MASTER_NUM; // I2C_NUM_0 or I2C_NUM_1
  INA219_instance_t INA219;
  INA219.I2C.device = &device;
  i2c_init(I2C_MODE_MASTER, I2C_MASTER_SDA_IO, I2C_MASTER_SCL_IO,
            GPIO_PULLUP_DISABLE, GPIO_PULLUP_DISABLE, I2C_MASTER_FREQ_HZ,
            &INA219.I2C);
  
  INA219.address = 0x40;
  INA219.calibration = CAL_32V_2A;
  INA219_init(&INA219);
```

Data collection
---------------

```c
  // read current from the INA219
  INA219.get_device_values(&INA219);
  ESP_LOGI("INA219", "Power:     %fmW", INA219.device_data.Power_mW);
  ESP_LOGI("INA219", "Current:   %fmA", INA219.device_data.Current_mA);
  ESP_LOGI("INA219", "Voltage:   %fV", INA219.device_data.Voltage_V);
  ESP_LOGI("INA219", "ShuntVoltage:   %fmV", INA219.device_data.ShuntVoltage_mV);

```


# Range & Limitations
The table below shows some ranges and characteristics, please refer to the [datasheet](https://www.ti.com/lit/ds/symlink/ina219.pdf?ts=1707265713398&ref_url=https%253A%252F%252Fwww.ti.com%252Fproduct%252FINA219%253Futm_source%253Dgoogle%2526utm_medium%253Dcpc%2526utm_campaign%253Dasc-amps-null-44700045336317092_prodfolderdynamic-cpc-pf-google-wwe_int%2526utm_content%253Dprodfolddynamic%2526ds_k%253DDYNAMIC%2BSEARCH%2BADS%2526DCM%253Dyes%2526gad_source%253D1%2526gclid%253DEAIaIQobChMIgL2D6fuXhAMV5RatBh2YrwioEAAYASAAEgK84fD_BwE%2526gclsrc%253Daw.ds) for more information.

ELECTRICAL CHARACTERISTICS
--------------------------

| Name                        | Range & Description                                      | Unit |
|-----------------------------|----------------------------------------------------------|------|
| Measurement Error           | ±0.2 -> ±1 depending on temperature                      | %    |
| Current Sense Voltage Range | ±40 -> ± ±320 depending on PGA set(1 to 8)               | mV   |
| Bus Voltage Range           | ±16 -> ±32 depending on BRNG (0 to 1)                    | V    |
| ADC Resolution              | 9 -> 12                                                  | bits |
| Voltage supply Range        | 3 -> 5.5                                                 | V    |
| I2C SCL Operating Frequency | (Fast mode) 0.001 -> 0.4, (HIGH-SPEED MODE) 0.001 -> 3.4 | MHz  |