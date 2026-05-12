if(CONFIG_ENABLE_NVS)
    list(APPEND srcs "${CMAKE_CURRENT_LIST_DIR}/nvs_read_write.c")
    list(APPEND includes "${CMAKE_CURRENT_LIST_DIR}/")
endif()