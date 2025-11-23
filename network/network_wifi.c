/**
 * @file network_wifi.c
 * @brief Implementation for Wi-Fi network interface
 */

#include "network.h"
#include "config.h"
#include "esp_log.h"
#include "esp_wifi.h"
#include "esp_netif.h"
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "lwip/sockets.h"
#include "lwip/netdb.h"
#include <string.h>
#include <fcntl.h>
#include <errno.h>

static const char *TAG = "net_wifi";
#define WIFI_CONNECTED_BIT BIT0
#define WIFI_FAIL_BIT      BIT1

static EventGroupHandle_t s_wifi_event_group;
static int s_retry_num = 0;
static int s_sock = -1;
static net_if_t s_current_iface = NET_IF_NONE;

/**
 * @brief Wi-Fi event handler
 * @param arg [in] user data (not used)
 * @param event_base [in] event type base (WIFI_EVENT/IP_EVENT)
 * @param event_id [in] specific event ID
 * @param event_data [in] pointer to event-specific data
 */
static void wifi_event_handler(void* arg, esp_event_base_t event_base,
                               int32_t event_id, void* event_data)
{
    (void)arg;
    if (event_base == WIFI_EVENT) {
        if (event_id == WIFI_EVENT_STA_START) {
            esp_wifi_connect();
            ESP_LOGI(TAG, "WIFI_EVENT_STA_START -> esp_wifi_connect()");
        } else if (event_id == WIFI_EVENT_STA_DISCONNECTED) {
            s_retry_num++;
            ESP_LOGW(TAG, "WiFi disconnected (retry %d)", s_retry_num);
            if (s_retry_num <= NET_MAX_RETRY) {
                esp_wifi_connect();
            } else {
                xEventGroupSetBits(s_wifi_event_group, WIFI_FAIL_BIT);
            }
        }
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data;
        ESP_LOGI(TAG, "Got IP: " IPSTR, IP2STR(&event->ip_info.ip));
        s_retry_num = 0;
        xEventGroupSetBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
    }
}

/**
 * @brief Initialize Wi-Fi interface (station mode)
 * @param type [in] network interface type, must be NET_IF_WIFI
 * @return true if Wi-Fi connected and IP obtained
 */
bool net_init(net_if_t type)
{
    if (type != NET_IF_WIFI) return false;
    if (s_current_iface == NET_IF_WIFI) return true;

    esp_netif_init();
    s_wifi_event_group = xEventGroupCreate();
    esp_netif_create_default_wifi_sta();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT,
                                                        ESP_EVENT_ANY_ID,
                                                        &wifi_event_handler,
                                                        NULL,
                                                        NULL));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT,
                                                        IP_EVENT_STA_GOT_IP,
                                                        &wifi_event_handler,
                                                        NULL,
                                                        NULL));

    wifi_config_t wifi_config = {
        .sta = {
            .ssid = WIFI_SSID,
            .password = WIFI_PASS,
            .threshold.authmode = WIFI_AUTH_WPA2_PSK,
            .pmf_cfg = { .capable = true, .required = false },
        },
    };

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());

    ESP_LOGI(TAG, "wifi_init_sta finished. Waiting for connection...");

    EventBits_t bits = xEventGroupWaitBits(s_wifi_event_group,
                                           WIFI_CONNECTED_BIT | WIFI_FAIL_BIT,
                                           pdFALSE, pdFALSE,
                                           pdMS_TO_TICKS(15000)); // 15s

    if (bits & WIFI_CONNECTED_BIT) {
        s_current_iface = NET_IF_WIFI;
        return true;
    } else {
        ESP_LOGE(TAG, "Failed to connect to SSID:%s", WIFI_SSID);
        return false;
    }
}

/**
 * @brief Check Wi-Fi link up status
 * @return true if Wi-Fi connected and has IP
 */
bool net_is_link_up(void)
{
    return (s_current_iface == NET_IF_WIFI) &&
           (xEventGroupGetBits(s_wifi_event_group) & WIFI_CONNECTED_BIT);
}

/**
 * @brief Connect TCP socket to host:port
 * @param host [in] hostname or IP
 * @param port [in] TCP port
 * @return true if socket connected, false otherwise
 */
bool net_connect(const char *host, int port)
{
    if (!net_is_link_up()) return false;

    struct addrinfo hints = {0}, *res = NULL;
    char port_str[8];
    snprintf(port_str, sizeof(port_str), "%d", port);
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;

    int err = getaddrinfo(host, port_str, &hints, &res);
    if (err != 0 || !res) {
        ESP_LOGE(TAG, "DNS lookup failed for %s", host);
        return false;
    }

    s_sock = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
    if (s_sock < 0) {
        ESP_LOGE(TAG, "Unable to create socket: errno %d", errno);
        freeaddrinfo(res);
        return false;
    }

    // non-blocking connect
    int flags = fcntl(s_sock, F_GETFL, 0);
    fcntl(s_sock, F_SETFL, flags | O_NONBLOCK);

    int ret = connect(s_sock, res->ai_addr, res->ai_addrlen);
    if (ret < 0 && errno != EINPROGRESS) {
        ESP_LOGE(TAG, "Socket connect error: errno %d", errno);
        close(s_sock); s_sock = -1;
        freeaddrinfo(res);
        return false;
    }

    freeaddrinfo(res);

    // select for timeout
    fd_set wfds;
    FD_ZERO(&wfds);
    FD_SET(s_sock, &wfds);
    struct timeval tv = { .tv_sec = 8, .tv_usec = 0 };
    ret = select(s_sock + 1, NULL, &wfds, NULL, &tv);
    if (ret <= 0) {
        ESP_LOGE(TAG, "connect timeout or select error");
        close(s_sock); s_sock = -1;
        return false;
    }

    int so_error = 0; socklen_t len = sizeof(so_error);
    getsockopt(s_sock, SOL_SOCKET, SO_ERROR, &so_error, &len);
    if (so_error != 0) {
        ESP_LOGE(TAG, "socket error after select: %d", so_error);
        close(s_sock); s_sock = -1;
        return false;
    }

    // set back blocking
    flags = fcntl(s_sock, F_GETFL, 0);
    fcntl(s_sock, F_SETFL, flags & ~O_NONBLOCK);

    ESP_LOGI(TAG, "TCP connected to %s:%d", host, port);
    return true;
}

/**
 * @brief Send data over TCP socket
 * @param data [in] pointer to buffer
 * @param len [in] number of bytes to send
 * @return number of bytes sent, -1 on error
 */
int net_send(const uint8_t *data, int len)
{
    if (s_sock < 0) return -1;
    int sent = send(s_sock, data, len, 0);
    if (sent < 0) ESP_LOGE(TAG, "send errno %d", errno);
    return sent;
}

/**
 * @brief Receive data from TCP socket with timeout
 * @param buf [out] buffer to store received data
 * @param max_len [in] max buffer length
 * @param timeout_ms [in] timeout in milliseconds
 * @return >0 bytes received, 0 peer closed, -1 error, -2 timeout
 */
int net_recv(uint8_t *buf, int max_len, uint32_t timeout_ms)
{
    if (s_sock < 0) return -1;

    struct timeval tv = { .tv_sec = timeout_ms / 1000, .tv_usec = (timeout_ms % 1000) * 1000 };
    setsockopt(s_sock, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));

    int r = recv(s_sock, buf, max_len, 0);
    if (r == 0) { close(s_sock); s_sock=-1; return 0; }
    else if (r < 0) {
        if (errno == EWOULDBLOCK || errno == EAGAIN) return -2; // timeout
        close(s_sock); s_sock=-1; return -1;
    }
    return r;
}

/**
 * @brief Disconnect TCP socket (does not disable Wi-Fi)
 */
void net_disconnect(void)
{
    if (s_sock >= 0) { close(s_sock); s_sock=-1; }
}

/**
 * @brief Switch interface at runtime
 * @param iface [in] target interface
 * @return true if switched successfully
 */
bool net_switch_interface(net_if_t iface)
{
    if (iface == NET_IF_WIFI) return net_init(NET_IF_WIFI);
    return false; // 4G not implemented
}
