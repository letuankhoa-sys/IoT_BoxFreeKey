#ifndef AWS_MQTT_CLIENT_H
#define AWS_MQTT_CLIENT_H

#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>


/**
 * @brief Initialize AWS MQTT client
 * @return true if initialization succeeded
 */
bool aws_mqtt_init(void);

/**
 * @brief Connect to AWS IoT broker
 * @return true if connection succeeded
 */
bool aws_mqtt_connect(void);

/**
 * @brief Disconnect MQTT client
 */
void aws_mqtt_disconnect(void);

/**
 * @brief Publish payload to topic
 * @param topic [in] MQTT topic
 * @param payload [in] pointer to data buffer
 * @param len [in] length of payload
 * @return true if publish succeeded
 */
extern bool aws_mqtt_publish(const char *topic, const uint8_t *payload, size_t len);

/**
 * @brief Subscribe to MQTT topic
 * @param topic [in] topic string
 * @param cb [in] callback function to handle incoming messages
 * @return true if subscription succeeded
 */
typedef void (*aws_msg_callback_t)(const char *topic, const uint8_t *payload, size_t len);
extern bool aws_mqtt_subscribe(const char *topic, aws_msg_callback_t cb);

/**
 * @brief Service MQTT client to process incoming messages
 * @param timeout_ms [in] maximum wait time for yielding
 */
void aws_mqtt_yield(uint32_t timeout_ms);

#endif // AWS_MQTT_CLIENT_H
