if(CONFIG_ENABLE_EPPP_CLIENT)
list(APPEND srcs "${CMAKE_CURRENT_LIST_DIR}/eppp_client.c")
list(APPEND includes "${CMAKE_CURRENT_LIST_DIR}/")
endif()
