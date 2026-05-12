if(CONFIG_ENABLE_HTTP_SERVER)
    list(APPEND srcs "${CMAKE_CURRENT_LIST_DIR}/http_server_local.c")
    list(APPEND includes "${CMAKE_CURRENT_LIST_DIR}/")
endif()