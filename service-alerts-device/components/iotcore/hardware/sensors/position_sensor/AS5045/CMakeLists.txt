if(CONFIG_ENABLE_AS5045)
    list(APPEND srcs    "${CMAKE_CURRENT_LIST_DIR}/AS5045.c")
    list(APPEND includes "${CMAKE_CURRENT_LIST_DIR}/")
endif()