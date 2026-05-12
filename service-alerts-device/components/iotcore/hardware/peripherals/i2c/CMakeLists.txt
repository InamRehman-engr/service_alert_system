if(CONFIG_HARDWARE_I2C)
list(APPEND srcs
    "${CMAKE_CURRENT_LIST_DIR}/i2c-dev.c"
)

list(APPEND includes
    "${CMAKE_CURRENT_LIST_DIR}/"
)
endif()