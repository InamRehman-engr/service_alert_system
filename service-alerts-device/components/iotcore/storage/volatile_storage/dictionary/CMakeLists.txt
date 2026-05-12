if(CONFIG_ENABLE_IN_MEMORY_DICTIONARY)
    list(APPEND srcs "${CMAKE_CURRENT_LIST_DIR}/dictionary.c")
    list(APPEND includes "${CMAKE_CURRENT_LIST_DIR}/")
endif()