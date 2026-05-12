set(src_dirs "")

if(CONFIG_UNIT_TEST_I2C)
    list(APPEND src_dirs "${CMAKE_CURRENT_LIST_DIR}/../hardware/peripherals/i2c/tests")

endif()
message("${src_dirs}")

idf_component_register(SRC_DIRS "${src_dirs}"
                    INCLUDE_DIRS "${src_dirs}"
                    REQUIRES unity iot-core)