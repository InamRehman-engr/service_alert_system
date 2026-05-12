if(CONFIG_ENABLE_ICM20602)
    list(APPEND srcs "${CMAKE_CURRENT_LIST_DIR}/icm20602.c")
    list(APPEND includes "${CMAKE_CURRENT_LIST_DIR}/")
endif()
