set(srcs "")

set(includes "")
set(priv_includes "")
set(embed_txtfiles "")

include(${CMAKE_CURRENT_LIST_DIR}/core/CMakeLists.txt)
include(${CMAKE_CURRENT_LIST_DIR}/utils/CMakeLists.txt)
include(${CMAKE_CURRENT_LIST_DIR}/hardware/CMakeLists.txt)
include(${CMAKE_CURRENT_LIST_DIR}/output_devices/CMakeLists.txt)
include(${CMAKE_CURRENT_LIST_DIR}/networking/CMakeLists.txt)
include(${CMAKE_CURRENT_LIST_DIR}/storage/CMakeLists.txt)
include(${CMAKE_CURRENT_LIST_DIR}/system/CMakeLists.txt)

set(cmps "driver" "nvs_flash" "esp_eth" "esp_timer"
        "esp_https_ota" "app_update" "esp_wifi" "wpa_supplicant" 
        "esp_http_server" "esp_http_client"
        "json" "mqtt" "bt" "console" "esp_adc" "spiffs" "fatfs"
    )

include(${CMAKE_CURRENT_LIST_DIR}/app/CMakeLists.txt)

if (CONFIG_ENABLE_IOTCORE_SERVER_API_EMBED_JSON)
    list(APPEND embed_txtfiles "embeddedresources/beef.txt")
endif()
if (CONFIG_ENABLE_NETWORK_UI)
    list(APPEND embed_txtfiles "networking/protocols/http/http_server/wifi.html" 
                                 )
endif()
if (CONFIG_ENABLE_FIRMWARE_UPLOAD_UI)
    list(APPEND embed_txtfiles "networking/protocols/http/http_server/ota_upload.html" 
                                 )
endif()

idf_component_register(SRCS ${srcs}
                        INCLUDE_DIRS ${includes}
                        REQUIRES "${cmps}"
                        PRIV_INCLUDE_DIRS ${priv_includes}
                        EMBED_TXTFILES ${embed_txtfiles}
                    )