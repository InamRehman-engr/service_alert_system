if(CONFIG_ENABLE_MAX30102)
    list(APPEND srcs "${CMAKE_CURRENT_LIST_DIR}/max30102.c" "${CMAKE_CURRENT_LIST_DIR}/spo2_algorithm.c")
    list(APPEND includes "${CMAKE_CURRENT_LIST_DIR}/")
endif()
