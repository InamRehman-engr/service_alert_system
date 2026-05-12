if(CONFIG_ENABLE_UTILITY)
    list(APPEND srcs "${CMAKE_CURRENT_LIST_DIR}/utility.c")
    list(APPEND includes "${CMAKE_CURRENT_LIST_DIR}/")
endif()