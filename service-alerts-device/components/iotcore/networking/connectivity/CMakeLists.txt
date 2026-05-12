include(${CMAKE_CURRENT_LIST_DIR}/eppp_client/CMakeLists.txt)
include(${CMAKE_CURRENT_LIST_DIR}/wifi/CMakeLists.txt)
include(${CMAKE_CURRENT_LIST_DIR}/ethernet/CMakeLists.txt)
include(${CMAKE_CURRENT_LIST_DIR}/ble/CMakeLists.txt)
include(${CMAKE_CURRENT_LIST_DIR}/modem/CMakeLists.txt)

include(${CMAKE_CURRENT_LIST_DIR}/network_ping/CMakeLists.txt)

list(APPEND srcs "${CMAKE_CURRENT_LIST_DIR}/connectivity.c")
list(APPEND includes "${CMAKE_CURRENT_LIST_DIR}/")