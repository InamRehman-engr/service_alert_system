#ifndef __system_status_reporting_h__
#define __system_status_reporting_h__

#include "esp_types.h"
/**
 * @brief Starts a task that reports events recieved from monitoring to the
 * server using mqtt events
 *
 */
void start_system_status_reporting();
#endif // __system_status_reporting_h__