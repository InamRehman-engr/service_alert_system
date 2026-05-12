if(CONFIG_ENABLE_BMP280)
    list(APPEND srcs    "${CMAKE_CURRENT_LIST_DIR}/bmp280.c")
    list(APPEND includes "${CMAKE_CURRENT_LIST_DIR}/")
endif()
