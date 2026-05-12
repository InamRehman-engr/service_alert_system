#include "esp_err.h"
#include "esp_log.h"
#include "esp_spiffs.h"
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/unistd.h>

static const char *TAG = "SPIFFS";

/**
 * Initialize and mount the SPIFFS file system.
 *
 * @param partition_label The name of the SPIFFS partition.
 * @param max_files Maximum files that could be open at the same time.
 * @param format_fs format the existing file system.
 * @return 0 if successful
 * -1 on error
 */
uint8_t spiffs_init(const char *partition_label, size_t max_files,
                    bool format_fs);

/**
 * Mount the SPIFFS file system.
 *
 * @return 0 if successful, -1 on error.
 */
int spiffs_mount(void);

/**
 * Unmount the SPIFFS file system.
 *
 * @return 0 if successful, -1 on error.
 */
int spiffs_unmount(void);

/**
 * Open a file for reading or writing.
 *
 * @param filename The name of the file to open.
 * @param mode     The file access mode ("r" for reading, "w" for writing, "a"
 * for append).
 * @return A file handle if successful, NULL on error.
 */
FILE *spiffs_fopen(const char *filename, const char *mode);

/**
 * Read data from an open file.
 *
 * @param file   The file handle.
 * @param buffer The buffer to store the read data.
 * @param size   The number of bytes to read.
 * @return The number of bytes read, or -1 on error.
 */
int spiffs_fread(FILE *file, void *buffer, size_t size);

/**
 * Write data to an open file.
 *
 * @param file   The file handle.
 * @param buffer The data to write.
 * @param size   The number of bytes to write.
 * @return The number of bytes written, or -1 on error.
 */
int spiffs_fwrite(FILE *file, const void *buffer, size_t size);

/**
 * Close an open file.
 *
 * @param file The file handle to close.
 * @return 0 if successful, -1 on error.
 */
int spiffs_fclose(FILE *file);

/**
 * Delete a file from the SPIFFS file system.
 *
 * @param filename The name of the file to delete.
 * @return 0 if successful, -1 on error.
 */
int spiffs_remove(const char *filename);

/**
 * Rename a file in the SPIFFS file system.
 *
 * @param old_name The current name of the file.
 * @param new_name The new name for the file.
 * @return 0 if successful, -1 on error.
 */
int spiffs_rename(const char *old_name, const char *new_name);

/**
 * Get information about a file in the SPIFFS file system.
 *
 * @param filename The name of the file to retrieve information about.
 * @param info     A structure to store the file information.
 * @return 0 if successful, -1 on error.
 */
int spiffs_stat(const char *filename, struct stat *info);
