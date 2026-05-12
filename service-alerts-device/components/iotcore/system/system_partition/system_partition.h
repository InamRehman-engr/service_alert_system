#ifndef __syspartition__
#define __syspartition__

#include "esp_err.h"
#include <stdint.h>

/**
 * @brief Set Boot Partition
 *
 * @param const char *partition_label
 * @return esp_err_t
 */
esp_err_t setBootPartition(const char *partition_label);
const char *get_running_partition_label();
#endif
