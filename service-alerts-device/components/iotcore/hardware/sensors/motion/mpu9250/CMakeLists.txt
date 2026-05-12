if(CONFIG_ENABLE_MPU9250)
    list(APPEND srcs    "${CMAKE_CURRENT_LIST_DIR}/mpu9250_accel.c"
                        "${CMAKE_CURRENT_LIST_DIR}/mpu9250_baro_calibrator.c"
                        "${CMAKE_CURRENT_LIST_DIR}/mpu9250_baro.c"
                        "${CMAKE_CURRENT_LIST_DIR}/mpu9250_calibrator.c"
                        "${CMAKE_CURRENT_LIST_DIR}/mpu9250_gyro.c"
                        "${CMAKE_CURRENT_LIST_DIR}/mpu9250_mag_calibrator.c"
                        "${CMAKE_CURRENT_LIST_DIR}/mpu9250_mag.c"
                        "${CMAKE_CURRENT_LIST_DIR}/mpu9250_spi.c"
                        "${CMAKE_CURRENT_LIST_DIR}/mpu9250.c"
                        "${CMAKE_CURRENT_LIST_DIR}/my_mpu9250_task.c"
                        )
    list(APPEND includes "${CMAKE_CURRENT_LIST_DIR}/")
endif()
