#ifndef __system_monitoring_h__
#define __system_monitoring_h__

/////////Goodby Protocol includes //////////
#include "esp_types.h"

/**
 * @brief Start good bye protocol. this is here for legacy support and should be
 * removed in favor of proper power management
 *
 * @param voltage this gets reported to the server
 */
void StartGoodByProtocol(float voltage, int32_t clientID,
                         int32_t *GOODByProtocolInInit,
                         const uint64_t ext_wakeup_pins_mask);

/**
 * @brief End good bye protocol. Only reports successful getting out of good bye
 * protocol
 *
 * @param voltage this gets reported to the server
 */
void EndGoodByProtocol(float voltage, int32_t *GOODByProtocolInInit);

void goToDeepSleep(const uint64_t ext_wakeup_pins_mask);

/**
 * @brief This function starts a thread which monitors the statistics and posts
 * them to the system event loop
 *
 */
void start_system_monitoring();
#endif // __system_monitoring_h__