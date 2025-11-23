#include "network_manager.h"
#include "network.h"
#include "aws_mqtt_client.h"
#include "config.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include <string.h>

static const char *TAG = "net_mgr";

/**
 * @brief Network manager state machine states
 */
typedef enum {
    NM_STATE_IDLE = 0,          /**< initial state */
    NM_STATE_NET_INIT,          /**< initializing network */
    NM_STATE_NET_CONNECTING,    /**< waiting for link up */
    NM_STATE_NET_CONNECTED,     /**< network ready, prepare MQTT */
    NM_STATE_MQTT_INIT,         /**< initialize MQTT client */
    NM_STATE_MQTT_CONNECTING,   /**< connect to AWS MQTT */
    NM_STATE_MQTT_CONNECTED,    /**< MQTT connected, ready to publish/subscribe */
    NM_STATE_ERROR              /**< error state, retry */
} nm_state_t;

/**
 * @brief Events for state machine
 */
typedef enum {
    NM_EVENT_NONE = 0,
    NM_EVENT_START,
    NM_EVENT_NET_UP,
    NM_EVENT_NET_DOWN,
    NM_EVENT_MQTT_CONNECTED,
    NM_EVENT_MQTT_DISCONNECTED,
    NM_EVENT_PUBLISH_REQ,
    NM_EVENT_STOP
} nm_event_t;

/**
 * @brief Queue message structure
 */
typedef struct {
    nm_event_t ev;              /**< event type */
    char payload[256];          /**< payload for publish requests */
} nm_msg_t;

#define NM_QUEUE_LEN 8
static QueueHandle_t nm_queue = NULL;
static TaskHandle_t nm_task_handle = NULL;

static nm_state_t s_state = NM_STATE_IDLE;
static int s_retry = 0;

/**
 * @brief Internal publish helper
 * @param payload [in] JSON string
 */
static void publish_internal(const char *payload)
{
    if (s_state != NM_STATE_MQTT_CONNECTED) {
        ESP_LOGW(TAG, "publish_internal: MQTT not connected");
        return;
    }
    aws_mqtt_publish(AWS_PUB_TOPIC, (const uint8_t*)payload, strlen(payload));
}

/**
 * @brief Callback for subscribed messages
 * @param topic [in] MQTT topic
 * @param payload [in] received payload
 * @param len [in] length of payload
 */
static void subscription_cb(const char *topic, const uint8_t *payload, size_t len)
{
    ESP_LOGI(TAG, "MSG FROM AWS topic=%s len=%d", topic, (int)len);
    // here you can parse command and act
}

/**
 * @brief State machine event handler
 * @param msg [in] message/event
 */
static void nm_handle_event(nm_msg_t *msg)
{
    switch (s_state) {
        case NM_STATE_IDLE:
            if (msg->ev == NM_EVENT_START) {
                ESP_LOGI(TAG, "State: IDLE -> NET_INIT");
                s_state = NM_STATE_NET_INIT;
            }
            break;
        case NM_STATE_NET_INIT:
            ESP_LOGI(TAG, "Initializing network (WiFi)");
            if (net_init(NET_IF_WIFI)) {
                s_retry = 0;
                s_state = NM_STATE_NET_CONNECTING;
            } else {
                ESP_LOGE(TAG, "net_init failed");
                s_state = NM_STATE_ERROR;
            }
            break;
        case NM_STATE_NET_CONNECTING:
            ESP_LOGI(TAG, "NET_CONNECTING: waiting link up...");
            if (net_is_link_up()) {
                ESP_LOGI(TAG, "Network link up");
                s_state = NM_STATE_NET_CONNECTED;
            } else {
                s_retry++;
                if (s_retry > NET_MAX_RETRY) {
                    ESP_LOGE(TAG, "Network connect retry exceeded");
                    s_state = NM_STATE_ERROR;
                } else {
                    vTaskDelay(pdMS_TO_TICKS(2000 * s_retry));
                }
            }
            break;
        case NM_STATE_NET_CONNECTED:
            ESP_LOGI(TAG, "State NET_CONNECTED -> MQTT_INIT");
            s_state = NM_STATE_MQTT_INIT;
            break;
        case NM_STATE_MQTT_INIT:
            if (!aws_mqtt_init()) {
                ESP_LOGE(TAG, "aws_mqtt_init failed");
                s_state = NM_STATE_ERROR;
            } else {
                s_state = NM_STATE_MQTT_CONNECTING;
            }
            break;
        case NM_STATE_MQTT_CONNECTING:
            if (!net_connect(AWS_IOT_HOST, AWS_IOT_PORT)) {
                ESP_LOGW(TAG, "TCP connect to AWS failed, retrying");
                s_retry++;
                if (s_retry > NET_MAX_RETRY) s_state = NM_STATE_ERROR;
                else vTaskDelay(pdMS_TO_TICKS(2000 * s_retry));
            } else {
                if (aws_mqtt_connect()) {
                    aws_mqtt_subscribe(AWS_SUB_TOPIC, subscription_cb);
                    s_state = NM_STATE_MQTT_CONNECTED;
                    s_retry = 0;
                } else {
                    ESP_LOGW(TAG, "aws_mqtt_connect failed");
                    net_disconnect();
                    s_retry++;
                    if (s_retry > NET_MAX_RETRY) s_state = NM_STATE_ERROR;
                    else vTaskDelay(pdMS_TO_TICKS(2000 * s_retry));
                }
            }
            break;
        case NM_STATE_MQTT_CONNECTED:
            // handle publish events
            if (msg->ev == NM_EVENT_PUBLISH_REQ) {
                publish_internal(msg->payload);
            } else if (msg->ev == NM_EVENT_NET_DOWN) {
                ESP_LOGW(TAG, "Network down while MQTT connected");
                aws_mqtt_disconnect();
                net_disconnect();
                s_state = NM_STATE_NET_INIT;
            }
            break;
        case NM_STATE_ERROR:
            ESP_LOGE(TAG, "State ERROR: retrying...");
            vTaskDelay(pdMS_TO_TICKS(5000));
            s_retry = 0;
            s_state = NM_STATE_NET_INIT;
            break;
        default:
            break;
    }
}

/**
 * @brief FreeRTOS task for network manager
 *        Processes events and handles periodic MQTT publish
 */
static void nm_task(void *arg)
{
    (void)arg;
    nm_msg_t msg;
    TickType_t last_publish = xTaskGetTickCount();

    while (1) {
        // process queue events
        if (xQueueReceive(nm_queue, &msg, pdMS_TO_TICKS(500))) {
            nm_handle_event(&msg);
        } else {
            // periodic actions
            if (s_state == NM_STATE_MQTT_CONNECTED) {
                aws_mqtt_yield(100);
                if (xTaskGetTickCount() - last_publish >= pdMS_TO_TICKS(NET_MANAGER_PUBLISH_INTERVAL_MS)) {
                    nm_msg_t pub;
                    pub.ev = NM_EVENT_PUBLISH_REQ;
                    int n = snprintf(pub.payload, sizeof(pub.payload), "{\"hello\":\"aws\",\"ts\":%d}", (int)xTaskGetTickCount());
                    if (n < (int)sizeof(pub.payload)) {
                        xQueueSend(nm_queue, &pub, 0);
                    }
                    last_publish = xTaskGetTickCount();
                }
            } else if (s_state == NM_STATE_NET_CONNECTING || s_state == NM_STATE_NET_INIT) {
                nm_msg_t poll = {.ev = NM_EVENT_NONE};
                nm_handle_event(&poll);
            }
        }
    }
}

/**
 * @brief Start network manager
 *        Creates FreeRTOS task and sends start event
 */
void network_manager_start(void)
{
    if (nm_queue) return;
    nm_queue = xQueueCreate(NM_QUEUE_LEN, sizeof(nm_msg_t));
    xTaskCreate(nm_task, "network_manager", 12*1024, NULL, 5, &nm_task_handle);

    nm_msg_t start = {.ev = NM_EVENT_START};
    xQueueSend(nm_queue, &start, pdMS_TO_TICKS(100));
}

/**
 * @brief Request immediate MQTT publish
 * @param payload [in] JSON string
 */
void network_manager_request_publish(const char *payload)
{
    if (!nm_queue) return;
    nm_msg_t m;
    m.ev = NM_EVENT_PUBLISH_REQ;
    strncpy(m.payload, payload, sizeof(m.payload)-1);
    m.payload[sizeof(m.payload)-1] = 0;
    xQueueSend(nm_queue, &m, pdMS_TO_TICKS(50));
}
