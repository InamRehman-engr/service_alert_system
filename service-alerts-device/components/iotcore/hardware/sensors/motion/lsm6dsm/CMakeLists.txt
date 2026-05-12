if(CONFIG_ENABLE_LSM6DSM)
    list(APPEND srcs "${CMAKE_CURRENT_LIST_DIR}/lsm6dsm_reg.c" "${CMAKE_CURRENT_LIST_DIR}/lsm6dsm.c")
    list(APPEND includes "${CMAKE_CURRENT_LIST_DIR}/")
endif()
