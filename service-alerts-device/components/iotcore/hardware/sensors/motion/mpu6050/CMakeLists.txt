if(CONFIG_ENABLE_MPU6050)
    list(APPEND srcs    "${CMAKE_CURRENT_LIST_DIR}/i2c_application.c" 
                        "${CMAKE_CURRENT_LIST_DIR}/i2c_driver.c" 
                        "${CMAKE_CURRENT_LIST_DIR}/kalman_filter.c" 
                        "${CMAKE_CURRENT_LIST_DIR}/mpu6050_application.c" 
                        "${CMAKE_CURRENT_LIST_DIR}/mpu6050.c"
                        )
    list(APPEND includes "${CMAKE_CURRENT_LIST_DIR}/")
endif()
