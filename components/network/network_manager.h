#ifndef NETWORK_MANAGER_H
#define NETWORK_MANAGER_H

#include <stdbool.h>

/**
 * @brief Start network manager task
 *        - Handles WiFi/4G network state
 *        - Connects MQTT to AWS
 *        - Periodic publish & subscription handling
 */
extern void network_manager_start(void);

/**
 * @brief Request MQTT publish
 * @param payload [in] JSON payload string
 */
extern void network_manager_request_publish(const char *payload);

#endif // NETWORK_MANAGER_H
