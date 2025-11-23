#include "aws_mqtt_client.h"
#include "config.h"
#include "esp_log.h"
#include <string.h>

static const char *TAG = "aws_mqtt";

static bool s_initialized = false;
static bool s_connected = false;
static aws_msg_callback_t s_sub_cb = NULL;

/**
 * @brief Initialize AWS MQTT client (load certs, init TLS etc)
 * @return true if init succeeded
 */
bool aws_mqtt_init(void)
{
    ESP_LOGI(TAG, "aws_mqtt_init: loading certs from: %s, %s, %s",
             AWS_ROOT_CA_PATH, AWS_CERT_PATH, AWS_PRIVATE_KEY_PATH);
    // TODO: load certificates and initialize TLS stack
    s_initialized = true;
    return true;
}

/**
 * @brief Connect to AWS IoT broker
 * @return true if MQTT connected
 */
bool aws_mqtt_connect(void)
{
    if (!s_initialized) {
        ESP_LOGE(TAG, "aws_mqtt_connect: not initialized");
        return false;
    }
    ESP_LOGI(TAG, "Connecting to AWS IoT: %s:%d", AWS_IOT_HOST, AWS_IOT_PORT);
    // TODO: call actual AWS SDK connect function
    s_connected = true;
    return true;
}

/**
 * @brief Disconnect MQTT client
 */
void aws_mqtt_disconnect(void)
{
    if (!s_connected) return;
    ESP_LOGI(TAG, "Disconnecting MQTT");
    // TODO: SDK disconnect
    s_connected = false;
}

/**
 * @brief Publish payload to MQTT topic
 * @param topic [in] topic string
 * @param payload [in] data buffer
 * @param len [in] buffer length
 * @return true if publish succeeded
 */
bool aws_mqtt_publish(const char *topic, const uint8_t *payload, size_t len)
{
    if (!s_connected) {
        ESP_LOGW(TAG, "Publish called but MQTT not connected");
        return false;
    }
    ESP_LOGI(TAG, "Publishing to topic=%s payload_len=%d", topic, (int)len);
    if (len < 256) {
        char tmp[256];
        memcpy(tmp, payload, len);
        tmp[len] = 0;
        ESP_LOGI(TAG, "Payload: %s", tmp);
    }
    // TODO: SDK publish
    return true;
}

/**
 * @brief Subscribe to topic with callback
 * @param topic [in] topic string
 * @param cb [in] callback function
 * @return true if subscription succeeded
 */
bool aws_mqtt_subscribe(const char *topic, aws_msg_callback_t cb)
{
    if (!s_connected) return false;
    s_sub_cb = cb;
    ESP_LOGI(TAG, "Subscribed to topic=%s (stub)", topic);
    // TODO: call SDK subscribe and link callback
    return true;
}

/**
 * @brief MQTT yield / service loop
 * @param timeout_ms [in] maximum wait time for yielding
 */
void aws_mqtt_yield(uint32_t timeout_ms)
{
    (void)timeout_ms;
    // TODO: call SDK yield to process incoming messages
    // Incoming message will trigger s_sub_cb
}
