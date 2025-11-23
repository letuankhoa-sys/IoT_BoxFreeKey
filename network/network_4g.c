/**
 * @file network_4g.c
 * @brief 4G/LTE network interface stub (to be implemented with module SIMxxx)
 */

#include "network.h"
#include "esp_log.h"

static const char *TAG = "net_4g_stub";

/**
 * @brief Initialize 4G interface (stub)
 * @param type [in] must be NET_IF_4G
 * @return false (stub)
 */
bool net_init(net_if_t type)
{
    if (type != NET_IF_4G) return false;
    ESP_LOGW(TAG, "4G not implemented yet (stub)");
    return false;
}

/**
 * @brief Check 4G link status (stub)
 * @return false
 */
bool net_is_link_up(void)
{
    return false;
}

/**
 * @brief Connect TCP via 4G (stub)
 * @param host [in] hostname or IP
 * @param port [in] TCP port
 * @return false
 */
bool net_connect(const char *host, int port)
{
    (void)host; (void)port;
    ESP_LOGW(TAG, "net_connect(4G) stub");
    return false;
}

/**
 * @brief Send data over 4G (stub)
 * @param data [in] buffer
 * @param len [in] length
 * @return -1
 */
int net_send(const uint8_t *data, int len)
{
    (void)data; (void)len;
    return -1;
}

/**
 * @brief Receive data over 4G (stub)
 * @param buf [out] buffer
 * @param max_len [in] max length
 * @param timeout_ms [in] timeout
 * @return -1
 */
int net_recv(uint8_t *buf, int max_len, uint32_t timeout_ms)
{
    (void)buf; (void)max_len; (void)timeout_ms;
    return -1;
}

/**
 * @brief Disconnect 4G (stub)
 */
void net_disconnect(void)
{
    // nothing
}

/**
 * @brief Switch interface to 4G (stub)
 * @param iface [in] target interface
 * @return false
 */
bool net_switch_interface(net_if_t iface)
{
    (void)iface;
    return false;
}
