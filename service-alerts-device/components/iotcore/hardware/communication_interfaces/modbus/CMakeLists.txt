if(CONFIG_ENABLE_MODBUS)
list(APPEND srcs "${CMAKE_CURRENT_LIST_DIR}/modbus.c")
list(APPEND includes "${CMAKE_CURRENT_LIST_DIR}/")
endif()
