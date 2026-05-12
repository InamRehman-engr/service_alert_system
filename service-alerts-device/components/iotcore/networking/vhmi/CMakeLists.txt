if(CONFIG_ENABLE_VHMI)
    list(APPEND srcs "${CMAKE_CURRENT_LIST_DIR}/vhmi.c")
    list(APPEND includes "${CMAKE_CURRENT_LIST_DIR}/")
endif()