if(CONFIG_ENABLE_SHTC3)
    list(APPEND srcs "${CMAKE_CURRENT_LIST_DIR}/shtc3.c")
    list(APPEND includes "${CMAKE_CURRENT_LIST_DIR}/")
endif()
