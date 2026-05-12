if(CONFIG_ENABLE_TSL2561)
    list(APPEND srcs "${CMAKE_CURRENT_LIST_DIR}/tsl2561.c")
    list(APPEND includes "${CMAKE_CURRENT_LIST_DIR}/")
endif()
