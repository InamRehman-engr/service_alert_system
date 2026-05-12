# IoT Core

![Build](https://img.shields.io/badge/Build-failing-orange)
![version](https://img.shields.io/badge/version-0.0.0-blue)
![coverage](https://img.shields.io/badge/coverage-0%25-yellowgreen)

## Table of Contents

### [App Iotcore](./app/Readme.md)

### Hardware

* **Communication Interfaces**
  * [Modbus](./hardware/communication_interfaces/modbus/README.md)
  * [RS485](./hardware/communication_interfaces/rs485/README.md)
* **Sensors**
  * [Air Quality Sensor](./hardware/sensors/air_quality/README.md)
  * [Motion Sensors](./hardware/sensors/motion/README.md)
  * [Current Sensor](./hardware/sensors/current/README.md)
  * [Gas Sensors](./hardware/sensors/gas/README.md)
  * [Heart Rate Monitor](./hardware/sensors/heart_rate_monitor/README.md)
  * Temperature & Humidity Sensors
  * [Light Sensors](./hardware/sensors/light/README.md)

* **Peripherals**
  * [Button](./hardware/peripherals/button/README.md)
  * [RTC](./hardware/peripherals/rtc/README.md)
  * [ADC](./hardware/peripherals/adc/README.md)
  * [GPIO](./hardware/peripherals/gpio/README.md)
  * [I2C](./hardware/peripherals/i2c/README.md)
  * [I2S](./hardware/peripherals/i2s/README.md)   ![Upcoming](https://img.shields.io/badge/upcoming%20feature-yellow)
  * [RMT](./hardware/peripherals/rmt/README.md)
  * [SPI](./hardware/peripherals/spi_master/README.md)
  * [UART](./hardware/peripherals/uart/README.md)

### Utils

* [JSON](./utils/json/README.md)
* [Error Handling](./utils/error_handling/README.md)
* [DC Codes](./utils/dc_codes/README.md)
* [Utility](./utils/utility/README.md)
* [URL Encoding](./utils/url_encoding/README.md)

### Networking

* **[Connectivity](./networking/connectivity/README.md)**

  * [BLE](./networking/connectivity/ble/README.md) ![Enchance](https://img.shields.io/badge/enhancement-blue)
  * [WIFI](./networking/connectivity/wifi/README.md)
  * Ethernet  ![Upcoming](https://img.shields.io/badge/upcoming%20feature-yellow)
  * GSM   ![Deprecated](https://img.shields.io/badge/deprecated-orange)

* **Protocols**
  * [HTTP](./networking/protocols/http/README.md)
  * [MQTT](./networking/protocols/mqtt/README.md)
  * [Sockets](./networking/protocols/sockets/README.md)

* [VHMI](./networking/vhmi/README.md)

### Output Devices

* **Displays**
  * [LCD](./output_devices/displays/lcd/README.md)   ![Upcoming](https://img.shields.io/badge/upcoming%20feature-yellow)
  * [LED](./output_devices/displays/led/README.md)
  * [LED Matrix](./output_devices//displays/led_matrix/README.md)   ![Upcoming](https://img.shields.io/badge/upcoming%20feature-yellow)
  * [OLED](./output_devices//displays//oled/README.md)   ![Upcoming](https://img.shields.io/badge/upcoming%20feature-yellow)
  * [SPI TFT](./output_devices/displays/spi_tft/README.md)   ![Upcoming](https://img.shields.io/badge/upcoming%20feature-yellow)
  * [VGA](./output_devices/displays/vga/README.md)   ![Upcoming](https://img.shields.io/badge/upcoming%20feature-yellow)

* **Sound**
  * [Audio Codecs](./output_devices/sound/audio_codecs/README.md)   ![Upcoming](https://img.shields.io/badge/upcoming%20feature-yellow)

### Core

* [Biling](./core/billing/README.md)
* [Battery Charging](./core/battery_charging/README.md)
* [HTTP Call](./core/http_call/README.md)
* Temperature and Humidity Sensor Comm. Interface Wrapper

### Storage

* **Database**
  * [SQLite](./storage/database/sqlite/README.md)   ![Upcoming](https://img.shields.io/badge/upcoming%20feature-yellow)
* **Non Volatile Storage**
  * [EEPROM](./storage/non_vol_storage/eeprom/README.md)   ![Upcoming](https://img.shields.io/badge/upcoming%20feature-yellow)
  * [NVS](./storage//non_vol_storage/nvs/README.md)
  * [SDCARD](./storage/non_vol_storage/sdcard/README.md)   ![Upcoming](https://img.shields.io/badge/upcoming%20feature-yellow)
  * [SPIFFS](./storage/non_vol_storage/spiffs/README.md)   ![Upcoming](https://img.shields.io/badge/upcoming%20feature-yellow)

* **Volatile Storage**
  * [In-memory Dictionary](./storage/volatile_storage/dictionary/README.md)

### System

* **Firmware Updates**
  * [OTA](./system/firmware_updates/ota/README.md)
* **Power Management**
  * [Sleep](./system/power_management/sleep/README.md)  ![Upcoming](https://img.shields.io/badge/upcoming%20feature-yellow)
* **Timing**
  * [Timers](./system/timing/timer/README.md)   ![Upcoming](https://img.shields.io/badge/upcoming%20feature-yellow)

### Testing

* [Unit Tests](./testing/unit_tests/README.md)    ![Test](https://img.shields.io/badge/test%20later-blue)
* [Integration Tests](./testing/integration_tests/README.md)     ![Test](https://img.shields.io/badge/test%20later-blue)
* [Test Data](./testing/test_data/README.md)

### Archives

* [Doc](./archives/README.md)

### Build and Deployment

* [Doc](./build_and_deployment/README.md)

### Config

* [Environment](./config/environment/README.md)
* [Settings](./config/settings/README.md)
* [Setup Scripts](./config/setup_scripts/README.md)

### Documentation

* [API](./documentation/api/README.md)
* [Design](./documentation/design/README.md)
* [User Manuals](./documentation/user_manuals/README.md)

### Issue Tracker

* [Doc](./issue_tracker/README.md)

### Scripts

* [Doc](./scripts/README.md)

### Web Assets

* [Doc](./web_assets/README.md)


### Dependency graph generation
 `python -m codeviz -r ./ --ignore=hardware/**/*.* --ignore=output_devices/**/*.*`