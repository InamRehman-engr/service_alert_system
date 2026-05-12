if(CONFIG_ENABLE_SBS_BATTERY)
    list(APPEND srcs "${CMAKE_CURRENT_LIST_DIR}/sbs_battery.c")
    list(APPEND includes "${CMAKE_CURRENT_LIST_DIR}/")
endif()