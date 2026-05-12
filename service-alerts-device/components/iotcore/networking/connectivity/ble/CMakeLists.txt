if(CONFIG_ENABLE_BLE)
    list(APPEND srcs "${CMAKE_CURRENT_LIST_DIR}/app_ble.c" "${CMAKE_CURRENT_LIST_DIR}/scli.c")
    list(APPEND includes "${CMAKE_CURRENT_LIST_DIR}/")
endif()