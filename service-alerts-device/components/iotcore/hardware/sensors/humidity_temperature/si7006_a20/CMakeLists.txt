if(CONFIG_ENABLE_SI7006_A20)
    list(APPEND srcs "${CMAKE_CURRENT_LIST_DIR}/si7006-a20.c")
    list(APPEND includes "${CMAKE_CURRENT_LIST_DIR}/")
endif()
