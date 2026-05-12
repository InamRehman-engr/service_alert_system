#include "i2c-dev.h"
#include "unity.h"

#define I2C_TEST_DEVICE_ADDRESS 0x60
TEST_CASE("I2C Init Function", "[I2C]") {
  i2c_functions i2c_obj;
  i2c_obj.device = malloc(sizeof(i2c_device_t));
  i2c_mode_t mode = I2C_MODE_MASTER;
  i2c_obj.device->port = I2C_NUM_0;
  i2c_init(mode, 26, 27, true, true, 200000, &i2c_obj);
  TEST_ASSERT_NOT_NULL(i2c_obj.device->i2c_mutex);
  TEST_ASSERT_NOT_NULL(i2c_obj.i2c_receive);
  TEST_ASSERT_NOT_NULL(i2c_obj.i2c_send);
  TEST_ASSERT_NOT_NULL(i2c_obj.i2c_send_receive);
#ifdef CONFIG_SMBUS_SUPPORT
  TEST_ASSERT_NOT_NULL(i2c_obj.smbus_send_byte);
  TEST_ASSERT_NOT_NULL(i2c_obj.smbus_receive_byte);
  TEST_ASSERT_NOT_NULL(i2c_obj.smbus_send_bytes);
  TEST_ASSERT_NOT_NULL(i2c_obj.smbus_receive_bytes);
  TEST_ASSERT_NOT_NULL(i2c_obj.smbus_send_block);
  TEST_ASSERT_NOT_NULL(i2c_obj.smbus_receive_block);
#endif
}

TEST_CASE("I2C Device Available Function", "[I2C]") {
  i2c_functions i2c_obj;
  i2c_obj.device = malloc(sizeof(i2c_device_t));
  i2c_mode_t mode = I2C_MODE_MASTER;
  i2c_obj.device->port = I2C_NUM_0;
  i2c_init(mode, 26, 27, true, true, 200000, &i2c_obj);
  TEST_ASSERT_EQUAL(ESP_OK, i2c_obj.device_available(I2C_TEST_DEVICE_ADDRESS,
                                                     i2c_obj.device));
}
TEST_CASE("I2C Send Function", "[I2C]") {
  i2c_functions i2c_obj;
  i2c_obj.device = malloc(sizeof(i2c_device_t));
  i2c_mode_t mode = I2C_MODE_MASTER;
  i2c_obj.device->port = I2C_NUM_0;
  i2c_init(mode, 26, 27, true, true, 200000, &i2c_obj);
  uint8_t data[2] = {0x20, 0x30};
  TEST_ASSERT_EQUAL(ESP_OK, i2c_obj.i2c_send(I2C_TEST_DEVICE_ADDRESS, data,
                                             sizeof(data), i2c_obj.device));
}

TEST_CASE("I2C Receive Function", "[I2C]") {
  i2c_functions i2c_obj;
  i2c_obj.device = malloc(sizeof(i2c_device_t));
  i2c_mode_t mode = I2C_MODE_MASTER;
  i2c_obj.device->port = I2C_NUM_0;
  i2c_init(mode, 26, 27, true, true, 200000, &i2c_obj);
  uint8_t data[12];
  TEST_ASSERT_EQUAL(ESP_OK, i2c_obj.i2c_receive(I2C_TEST_DEVICE_ADDRESS, data,
                                                12, i2c_obj.device));
  TEST_ASSERT_NOT_NULL(data);
}

TEST_CASE("I2C Send Recieve Function", "[I2C]") {
  i2c_functions i2c_obj;
  i2c_obj.device = malloc(sizeof(i2c_device_t));
  i2c_mode_t mode = I2C_MODE_MASTER;
  i2c_obj.device->port = I2C_NUM_0;
  i2c_init(mode, 26, 27, true, true, 200000, &i2c_obj);
  uint8_t data_send[2] = {0x20, 0x30};
  uint8_t data_receive[12];
  TEST_ASSERT_EQUAL(ESP_OK,
                    i2c_obj.i2c_send_receive(I2C_TEST_DEVICE_ADDRESS, data_send,
                                             sizeof(data_send), data_receive,
                                             12, i2c_obj.device));
  TEST_ASSERT_NOT_NULL(data_receive);
}

#ifdef CONFIG_SMBUS_SUPPORT
TEST_CASE("SMBUS Send Byte Function", "[I2C]") {
  i2c_functions i2c_obj;
  i2c_obj.device = malloc(sizeof(i2c_device_t));
  i2c_mode_t mode = I2C_MODE_MASTER;
  i2c_obj.device->port = I2C_NUM_0;
  i2c_init(mode, 26, 27, true, true, 200000, &i2c_obj);
  uint8_t data = 0x89;
  TEST_ASSERT_EQUAL(ESP_OK, i2c_obj.smbus_send_byte(I2C_TEST_DEVICE_ADDRESS,
                                                    data, i2c_obj.device));
}

TEST_CASE("SMBUS Receive Byte Function", "[I2C]") {
  i2c_functions i2c_obj;
  i2c_obj.device = malloc(sizeof(i2c_device_t));
  i2c_mode_t mode = I2C_MODE_MASTER;
  i2c_obj.device->port = I2C_NUM_0;
  i2c_init(mode, 26, 27, true, true, 200000, &i2c_obj);
  uint8_t *data;
  TEST_ASSERT_EQUAL(ESP_OK, i2c_obj.smbus_receive_byte(I2C_TEST_DEVICE_ADDRESS,
                                                       data, i2c_obj.device));
  TEST_ASSERT_NOT_NULL(data);
}
TEST_CASE("SMBUS Send Bytes Function", "[I2C]") {
  i2c_functions i2c_obj;
  i2c_obj.device = malloc(sizeof(i2c_device_t));
  i2c_mode_t mode = I2C_MODE_MASTER;
  i2c_obj.device->port = I2C_NUM_0;
  i2c_init(mode, 26, 27, true, true, 200000, &i2c_obj);
  uint8_t data[3] = {0x50, 0x40, 0x54};
  TEST_ASSERT_EQUAL(ESP_OK, i2c_obj.smbus_send_bytes(I2C_TEST_DEVICE_ADDRESS,
                                                     data, 3, i2c_obj.device));
}

TEST_CASE("SMBUS Receive Bytes Function", "[I2C]") {
  i2c_functions i2c_obj;
  i2c_obj.device = malloc(sizeof(i2c_device_t));
  i2c_mode_t mode = I2C_MODE_MASTER;
  i2c_obj.device->port = I2C_NUM_0;
  i2c_init(mode, 26, 27, true, true, 200000, &i2c_obj);
  uint8_t data[3];
  TEST_ASSERT_EQUAL(ESP_OK,
                    i2c_obj.smbus_receive_bytes(I2C_TEST_DEVICE_ADDRESS, data,
                                                3, i2c_obj.device));
  TEST_ASSERT_NOT_NULL(data);
}

TEST_CASE("SMBUS Send Bytes Function", "[I2C]") {
  i2c_functions i2c_obj;
  i2c_obj.device = malloc(sizeof(i2c_device_t));
  i2c_mode_t mode = I2C_MODE_MASTER;
  i2c_obj.device->port = I2C_NUM_0;
  i2c_init(mode, 26, 27, true, true, 200000, &i2c_obj);
  uint8_t data[3] = {0x50, 0x40, 0x54};
  TEST_ASSERT_EQUAL(ESP_OK, i2c_obj.smbus_send_bytes(I2C_TEST_DEVICE_ADDRESS,
                                                     data, 3, i2c_obj.device));
}

TEST_CASE("SMBUS Send Block Function", "[I2C]") {
  i2c_functions i2c_obj;
  i2c_obj.device = malloc(sizeof(i2c_device_t));
  i2c_mode_t mode = I2C_MODE_MASTER;
  i2c_obj.device->port = I2C_NUM_0;
  i2c_init(mode, 26, 27, true, true, 200000, &i2c_obj);
  uint8_t data[3] = {0x50, 0x40, 0x54};
  uint8_t cmd = 0x06;
  TEST_ASSERT_EQUAL(ESP_OK,
                    i2c_obj.smbus_send_block(I2C_TEST_DEVICE_ADDRESS, cmd, data,
                                             3, i2c_obj.device));
}

TEST_CASE("SMBUS Receive Block Function", "[I2C]") {
  i2c_functions i2c_obj;
  i2c_obj.device = malloc(sizeof(i2c_device_t));
  i2c_mode_t mode = I2C_MODE_MASTER;
  i2c_obj.device->port = I2C_NUM_0;
  i2c_init(mode, 26, 27, true, true, 200000, &i2c_obj);
  uint8_t *len;
  uint8_t data[3];
  uint8_t cmd = 0x06;
  TEST_ASSERT_EQUAL(ESP_OK,
                    i2c_obj.smbus_receive_block(I2C_TEST_DEVICE_ADDRESS, cmd,
                                                data, len, i2c_obj.device));
  TEST_ASSERT_NOT_EQUAL(*len, 0);
  TEST_ASSERT_NOT_NULL(data);
}
#endif