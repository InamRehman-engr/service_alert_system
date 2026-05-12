if(CONFIG_ENABLE_CURRENT_TRANSFORMER)
    list(APPEND srcs "${CMAKE_CURRENT_LIST_DIR}/current_transformer.c")
    list(APPEND includes "${CMAKE_CURRENT_LIST_DIR}/")
endif()
