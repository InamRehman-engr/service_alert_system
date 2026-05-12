if(CONFIG_ENABLE_OTA)
    list(APPEND srcs "${CMAKE_CURRENT_LIST_DIR}/base64_decoding.c")
    list(APPEND includes "${CMAKE_CURRENT_LIST_DIR}/")
endif()