if(CONFIG_ENABLE_MQTT)
    list(APPEND srcs "${CMAKE_CURRENT_LIST_DIR}/app_mqtt.c")
    list(APPEND includes "${CMAKE_CURRENT_LIST_DIR}/")
endif()