if(CONFIG_ENABLE_EPPP_LINK)
    list(APPEND srcs "${CMAKE_CURRENT_LIST_DIR}/eppp_link.c")
    list(APPEND includes "${CMAKE_CURRENT_LIST_DIR}/")
endif()