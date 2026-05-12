if(CONFIG_ENABLE_IOTCORE_EVENTS)
    list(APPEND srcs "${CMAKE_CURRENT_LIST_DIR}/iotcore_events.c")
    list(APPEND includes "${CMAKE_CURRENT_LIST_DIR}/")

    include(${CMAKE_CURRENT_LIST_DIR}/app_events/CMakeLists.txt)
    include(${CMAKE_CURRENT_LIST_DIR}/error_events/CMakeLists.txt)
    include(${CMAKE_CURRENT_LIST_DIR}/data_events/CMakeLists.txt)
endif()