if(CONFIG_ENABLE_IOTCORE_SERVER_API)
    list(APPEND srcs "${CMAKE_CURRENT_LIST_DIR}/iotcore_server_apis.c")
    list(APPEND includes "${CMAKE_CURRENT_LIST_DIR}/")
endif()