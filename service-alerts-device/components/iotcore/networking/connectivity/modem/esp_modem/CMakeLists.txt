idf_build_get_property(target IDF_TARGET)

if(${target} STREQUAL "linux")
    set(platform_srcs ${CMAKE_CURRENT_LIST_DIR}/src/esp_modem_primitives_linux.cpp
        ${CMAKE_CURRENT_LIST_DIR}/src/esp_modem_uart_linux.cpp
        ${CMAKE_CURRENT_LIST_DIR}/src/esp_modem_netif_linux.cpp)
    set(dependencies esp_system_protocols_linux)
else()
    set(platform_srcs ${CMAKE_CURRENT_LIST_DIR}/src/esp_modem_primitives_freertos.cpp
                      ${CMAKE_CURRENT_LIST_DIR}/src/esp_modem_api_target.cpp
                      ${CMAKE_CURRENT_LIST_DIR}/src/esp_modem_uart.cpp
                      ${CMAKE_CURRENT_LIST_DIR}/src/esp_modem_term_uart.cpp
                      ${CMAKE_CURRENT_LIST_DIR}/src/esp_modem_netif.cpp)
    set(dependencies driver esp_event esp_netif)
endif()


list(APPEND srcs ${platform_srcs}
            "${CMAKE_CURRENT_LIST_DIR}/src/esp_modem_dte.cpp"
            "${CMAKE_CURRENT_LIST_DIR}/src/esp_modem_dce.cpp"
            "${CMAKE_CURRENT_LIST_DIR}/src/esp_modem_api.cpp"
            "${CMAKE_CURRENT_LIST_DIR}/src/esp_modem_c_api.cpp"
            "${CMAKE_CURRENT_LIST_DIR}/src/esp_modem_factory.cpp"
            "${CMAKE_CURRENT_LIST_DIR}/src/esp_modem_cmux.cpp"
            "${CMAKE_CURRENT_LIST_DIR}/src/esp_modem_command_library.cpp"
            "${CMAKE_CURRENT_LIST_DIR}/src/esp_modem_term_fs.cpp"
            "${CMAKE_CURRENT_LIST_DIR}/src/esp_modem_vfs_uart_creator.cpp"
            "${CMAKE_CURRENT_LIST_DIR}/src/esp_modem_vfs_socket_creator.cpp"
            "${CMAKE_CURRENT_LIST_DIR}/src/esp_modem_gnss_parser.cpp"
            "${CMAKE_CURRENT_LIST_DIR}/src/esp_modem_modules.cpp")

list(APPEND includes "${CMAKE_CURRENT_LIST_DIR}/include")

list(APPEND priv_includes "${CMAKE_CURRENT_LIST_DIR}/private_include/")


set_target_properties(${COMPONENT_LIB} PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED ON
    CXX_EXTENSIONS ON
)

if(CONFIG_ESP_MODEM_ADD_CUSTOM_MODULE)
    idf_component_optional_requires(PUBLIC main)
endif()

if(${target} STREQUAL "linux")
    # This is needed for ESP_LOGx() macros, as integer formats differ on ESP32(..) and x64
    set_target_properties(${COMPONENT_LIB} PROPERTIES COMPILE_FLAGS -Wno-format)
endif()
