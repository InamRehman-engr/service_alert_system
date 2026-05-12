if(CONFIG_ENABLE_BATTERY_CHARGING)
    list(APPEND srcs "${CMAKE_CURRENT_LIST_DIR}/battery_charging.c")
    list(APPEND includes "${CMAKE_CURRENT_LIST_DIR}/")
endif()