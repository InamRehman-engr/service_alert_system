if(CONFIG_ENABLE_EPPP_SERVER)
    list(APPEND srcs "${CMAKE_CURRENT_LIST_DIR}/eppp_server.c")
    list(APPEND includes "${CMAKE_CURRENT_LIST_DIR}/")
endif()