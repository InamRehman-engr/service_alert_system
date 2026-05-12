if(CONFIG_ENABLE_JSON)
    list(APPEND srcs "${CMAKE_CURRENT_LIST_DIR}/myJSON.c")
    list(APPEND includes "${CMAKE_CURRENT_LIST_DIR}/")
endif()