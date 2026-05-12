if(CONFIG_ENABLE_LED)
    list(APPEND srcs "${CMAKE_CURRENT_LIST_DIR}/app_led.c")
    list(APPEND includes "${CMAKE_CURRENT_LIST_DIR}/")
endif()