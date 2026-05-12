if(CONFIG_HARDWARE_SPI_MASTER)
list(APPEND srcs
    "${CMAKE_CURRENT_LIST_DIR}/src/spi_master_dev.c"
)

list(APPEND includes
    "${CMAKE_CURRENT_LIST_DIR}/include/"
)
endif()