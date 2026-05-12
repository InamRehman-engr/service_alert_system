if(CONFIG_ENABLE_SYSTEM_STATUS_REPORTING)
    list(APPEND srcs "${CMAKE_CURRENT_LIST_DIR}/system_status_reporting.c")
    list(APPEND includes "${CMAKE_CURRENT_LIST_DIR}/")
endif()