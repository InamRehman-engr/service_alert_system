if(CONFIG_ENABLE_URL_ENCODING)
    list(APPEND srcs "${CMAKE_CURRENT_LIST_DIR}/url_encoding.c")
    list(APPEND includes "${CMAKE_CURRENT_LIST_DIR}/")
endif()