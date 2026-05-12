if(CONFIG_ENABLE_HARDWARE_UART)
    list(APPEND srcs "${CMAKE_CURRENT_LIST_DIR}/uart_driver.c")
    list(APPEND includes "${CMAKE_CURRENT_LIST_DIR}/")
endif()