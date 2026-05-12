if(CONFIG_ENABLE_ETHERNET)
list(APPEND srcs "${CMAKE_CURRENT_LIST_DIR}/ethernet.c")
list(APPEND includes "${CMAKE_CURRENT_LIST_DIR}/")
endif()
