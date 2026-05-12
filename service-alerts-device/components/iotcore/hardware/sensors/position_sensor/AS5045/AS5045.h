#ifndef _AS5045_H
#define _AS5045_H
#include <spi_master_dev.h>

/**
 * @brief This is a function available in iot-core to get latest data from the
 * sensor.
 * @param spi_device_handle is the pointer to the spi device handle pointer.
 * @param angle is the pointer to the angle to return the angle.
 * @param OCF is the pointer to the (Offset Compensation Finished) flag, logic
 * high indicates the finished Offset Compensation.
 * @param COF is the pointer to the (Cordic Overflow) flag, logic high indicates
 * an out of range error in the CORDIC part. When this bit is set, the data at
 * D9:D0 is invalid.
 * @param LIN is the pointer to the (Linearity Alarm) flag, logic high indicates
 * that the input field generates a critical output linearity, When this bit is
 * set, the data at D9:D0 may still be used, but can contain invalid data.
 * @param MAG_INC is the pointer to the magnetic increase bit.
 * @param MAG_DEC is the pointer to the magnetic decrease bit.
 * @param EVEN_PAR is the pointer to the parity bit checking even parity.
 */
esp_err_t as5045_encoder_data(spi_master_functions *spi_device_handle,
                              float *angle, bool *OCF, bool *COF, bool *LIN,
                              bool *MAG_INC, bool *MAG_DEC, bool *EVEN_PAR);
/**
 * @brief This is a function available in iot-core to configure the sensor to
 * your requirements.
 * @param spi_device_handle is the pointer to the spi device handle pointer.
 * @param angle is the angle to update zero position to.
 * @param CCW is the rotation direction.
 * @param PWM_DIS is the pwm disable flag.
 * @param MAG_COMP_EN is the magnetic field comparison to detect increase or
 * decrease in magnetic field.
 * @param PWM_HALF_EN is the enable or disable PWM.
 */
esp_err_t as5045_encoder_configurations(spi_master_functions *spi_device_handle,
                                        float angle, bool CCW, bool PWM_DIS,
                                        bool MAG_COMP_EN, bool PWM_HALF_EN);

#endif