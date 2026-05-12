if(CONFIG_HARDWARE_SPI_MASTER)
list(APPEND srcs
    "src/qspi-dev.c"
)

list(APPEND includes
    "include/"
)
endif()