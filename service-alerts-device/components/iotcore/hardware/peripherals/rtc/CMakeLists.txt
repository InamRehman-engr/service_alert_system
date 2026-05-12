if(CONFIG_ENABLE_APP_RTC)
list(APPEND srcs "${CMAKE_CURRENT_LIST_DIR}/app_rtc.c")
list(APPEND includes "${CMAKE_CURRENT_LIST_DIR}/")
endif()