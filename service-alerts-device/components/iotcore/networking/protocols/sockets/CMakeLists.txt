if(CONFIG_ENABLE_SOCKETS)
    list(APPEND srcs "${CMAKE_CURRENT_LIST_DIR}/tcp_server.c")
    list(APPEND includes "${CMAKE_CURRENT_LIST_DIR}/")
endif()