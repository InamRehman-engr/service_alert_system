if(CONFIG_ENABLE_SDCARD)
list(APPEND srcs
    "${CMAKE_CURRENT_LIST_DIR}/sd_card.c")
list(APPEND includes
    "${CMAKE_CURRENT_LIST_DIR}/"
)
endif()