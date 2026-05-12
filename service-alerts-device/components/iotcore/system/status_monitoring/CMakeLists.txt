if(CONFIG_ENABLE_SYSTEM_MONITORING)
    list(APPEND srcs "${CMAKE_CURRENT_LIST_DIR}/system_monitoring.c")
    list(APPEND includes "${CMAKE_CURRENT_LIST_DIR}/")
endif()