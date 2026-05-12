```C
#Example Code
#include "i2c-dev.h"
#include "icm20602.h"
#define ICM20602_ADDR 0x68
i2c_functions i2c_obj;
void app_main(){
    i2c_obj.device = malloc(sizeof(i2c_device_t));
    i2c_mode_t mode = I2C_MODE_MASTER;
    i2c_obj.device->port = I2C_NUM_0;
    i2c_init(mode, CONFIG_BUS1_I2C_MASTER_SDA, CONFIG_BUS1_I2C_MASTER_SCL, CONFIG_BUS1_SDA_PULLUP_EN, CONFIG_BUS1_SCL_PULLUP_EN, 100000, &i2c_obj);
    
    struct icm20602_dev icm_dev = {
        .id =  ICM20602_ADDR,
        .accel_g = ICM20602_ACCEL_RANGE_2G,
        .mutex_lock = NULL,
        .mutex_unlock = NULL,
        .i2c_obj = &i2c_obj,
        .use_accel = true,
        .use_gyro = false,
        .accel_fifo = false,
    };
    if (icm20602_init(&icm_dev) == !0)
    {
        printf("Init failed");
    }
    vTaskDelay(pdMS_TO_TICKS(100));
    float a_x, a_y, a_z;
    while(1){
        if (icm20602_read_accel(&icm_dev, &a_x, &a_y, &a_z) ==0)
        {
        printf("Acc X = %f  ", a_x);
        printf("Acc Y = %f  ", a_y);
        printf("Acc Z = %f\n", a_z);
        }
        else 
        {
            printf("Failed to read values\n");
        }
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}