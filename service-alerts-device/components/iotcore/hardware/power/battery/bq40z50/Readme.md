## Usage

### Initializing the Battery Wrapper
1. Go to menuconfig->component config -> IoT Core -> Drivers & Hardware Config -> Hardware -> Hardware Peripherals -> I2C -> Enabl I2C Hardware -> I2C Configuration -> I2C BUS 1
2. Configure the I2C Bus 1 that is to be used with the SBS battery
3. Also Enable the SMBUS in I2C Configuration
4. Go to menuconfig->component config -> IoT Core -> Drivers & Hardware Config -> Hardware -> Power Configuration -> Enable SBS Battery
5. Set the Battery I2C Address with Battery BQ Address.
6. In your ESP32 application code, initialize the battery wrapper as follows:

    ```c
    #include "sbs_battery.h"
    
    void app_main() {
        SemaphoreHandle_t read_write_mutex = xSemaphoreCreateMutex();
        battery_data data;
        memset(&data, 0, sizeof(battery_data));
        sbs_battery_init(&data, &read_write_mutex);
        
        while (1) {
            // Your code here
        }
    }

### Accessing Battery Information
    ```c
    if (xSemaphoreTake(read_write_mutex, pdMS_TO_TICKS(500)) == pdTRUE) {
        // Access battery data here

        xSemaphoreGive(read_write_mutex);
    }
