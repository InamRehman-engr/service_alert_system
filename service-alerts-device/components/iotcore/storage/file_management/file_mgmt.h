#pragma once
#include "esp_littlefs.h"

/**
 * Before using this class, include component littlefs:
 github.com/joltwallet/esp_littlefs.git
 * Example configuration:
 * esp_vfs_littlefs_conf_t conf = {
            .base_path = "/storage",
            .partition_label = "storage",
            .format_if_mount_failed = true,
            .dont_mount = false,
        };
 * Add partition in the partition table (partitions.csv):
 *     storage,    data, littlefs,    OFFSET,  SIZE_OF_PARTITION,
 * To flash the littlefs patition, include the following line in the
 CMakeList.txt of main folder:
 * littlefs_create_partition_image(storage_partition_name
 ../some_folder/storage_folder_location FLASH_IN_PROJECT)
*/

/**
 *  @param conf Address to the file in partition
 **/
void init_littlefs(esp_vfs_littlefs_conf_t *conf);
/**
 *  @param conf Address to the file in partition
 **/
void deinit_littlefs(esp_vfs_littlefs_conf_t *conf);
/**
 * @param path Path to the file in partition
 */
esp_err_t create_file_in_littlefs(const char *path);
/**
 *  @param path Address to the file in partition
 **/
esp_err_t delete_file_from_littlefs(const char *path);
/**
 *  @param path Address to the file in partition
 **/
bool file_exists_in_littlefs(const char *path);
/**
 *  @param path Address to the file in partition
 *  @param file_size Pointer to the file size
 **/
esp_err_t get_file_size(const char *path, size_t *file_size);
/**
 *  @param path Address to the file in partition
 *  @param data Pointer to the data that needs to be stored
 *  @param data_size Size of data
 **/
esp_err_t write_file_data(const char *path, const char *data, size_t data_size);
/**
 *  @param path Address to the file in partition you want to append
 *  @param data Pointer to the data that needs to be appended
 *  @param data_size Size of data
 * */
esp_err_t append_data_to_file(const char *path, const char *data,
                              size_t data_size);
/**
 * @param path Address to the file in partition
 * @param data Pointer to the data that needs to be stored
 * @param data_size Size of data
 * @param offset offset to write data, starting from 0
 * */
esp_err_t write_file_data_at_offset(const char *path, const char *data,
                                    size_t data_size, uint32_t offset);
/**
 * @param path Address to the file in partition
 * @param buffer returns dynamic string
 * @param read_data_length Pointer to length of read buffer
 * */
esp_err_t read_file_data(const char *path, unsigned char *buffer,
                         size_t *read_data_length);
/**
 * @param path Address to the file in partition
 * @param buffer returns dynamic string
 * @param read_data_length Pointer to length of read buffer
 * @param offset offset to write data, starting from 0
 * */
esp_err_t read_file_data_at_offset(const char *path, unsigned char *buffer,
                                   size_t *read_data_length, uint32_t offset);