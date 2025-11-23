#ifndef NETWORK_H
#define NETWORK_H

#include <stdbool.h>
#include <stdint.h>

/**
 * @brief Network interface type
 */
typedef enum {
    NET_IF_NONE = 0,   /**< No interface selected */
    NET_IF_WIFI,       /**< WiFi interface */
    NET_IF_4G          /**< 4G/LTE interface */
} net_if_t;

/**
 * @brief Network send result
 */
typedef enum {
    NET_SEND_OK = 0,   /**< send successful */
    NET_SEND_ERR = -1  /**< send failed */
} net_send_ret_t;

/**
 * @brief Initialize network stack
 * @param type [in] network interface type (NET_IF_WIFI or NET_IF_4G)
 * @return true if initialization succeeded, false otherwise
 */
bool net_init(net_if_t type);

/**
 * @brief Check whether network link is up
 * @return true if network has IP and can reach internet
 */
bool net_is_link_up(void);

/**
 * @brief Ensure TCP connectivity to host:port
 * @param host [in] hostname or IP
 * @param port [in] TCP port
 * @return true if connection succeeded, false otherwise
 */
bool net_connect(const char *host, int port);

/**
 * @brief Low-level send data
 * @param data [in] pointer to buffer to send
 * @param len [in] number of bytes to send
 * @return number of bytes sent, -1 on error
 */
int  net_send(const uint8_t *data, int len);

/**
 * @brief Receive data from network with timeout
 * @param buf [out] buffer to store received data
 * @param max_len [in] max buffer length
 * @param timeout_ms [in] timeout in milliseconds
 * @return number of bytes received (>0), 0 if peer closed, -1 on error, -2 on timeout
 */
int  net_recv(uint8_t *buf, int max_len, uint32_t timeout_ms);

/**
 * @brief Disconnect current network socket (does not disable interface)
 */
void net_disconnect(void);

/**
 * @brief Switch runtime network interface
 * @param iface [in] target network interface
 * @return true if switch succeeded
 */
bool net_switch_interface(net_if_t iface);

#endif // NETWORK_H
