if(CONFIG_ENABLE_PMS5003)
    list(APPEND srcs "${CMAKE_CURRENT_LIST_DIR}/pms5003.c")
    list(APPEND includes "${CMAKE_CURRENT_LIST_DIR}/")
endif()
