if(CONFIG_HARDWARE_ADC)
list(APPEND srcs
    "${CMAKE_CURRENT_LIST_DIR}/adc-dev.c"
)

list(APPEND includes
    "${CMAKE_CURRENT_LIST_DIR}/"
)
endif()