if(CONFIG_ENABLE_INA226)
    list(APPEND srcs "${CMAKE_CURRENT_LIST_DIR}/ina226.c")
    list(APPEND includes "${CMAKE_CURRENT_LIST_DIR}/")
endif()