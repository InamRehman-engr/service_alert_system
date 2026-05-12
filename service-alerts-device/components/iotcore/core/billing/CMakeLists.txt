if(CONFIG_ENABLE_BILLING)
    list(APPEND srcs "${CMAKE_CURRENT_LIST_DIR}/billing.c")
    list(APPEND includes "${CMAKE_CURRENT_LIST_DIR}/")
endif()