## Getting started
Before using littlefs component you need to add two lines mentioned below to you main/CMakeLists.txt

include(${CMAKE_SOURCE_DIR}/components/iot-core/storage/file_management/littlefs/project_include.cmake)
littlefs_create_partition_image({$partition-name} {$partition-path} FLASH_IN_PROJECT)