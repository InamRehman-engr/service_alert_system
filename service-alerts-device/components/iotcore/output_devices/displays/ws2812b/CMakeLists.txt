if(CONFIG_ENABLE_WS2812B)
    list(APPEND srcs "${CMAKE_CURRENT_LIST_DIR}/ws2812b_led_strip.c")
    list(APPEND includes "${CMAKE_CURRENT_LIST_DIR}/")
endif()