if(CONFIG_ENABLE_MAX31855K)
    list(APPEND srcs "${CMAKE_CURRENT_LIST_DIR}/max31855k.c")
    list(APPEND includes "${CMAKE_CURRENT_LIST_DIR}/")
endif()
