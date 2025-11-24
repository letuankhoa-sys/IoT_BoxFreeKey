#ifndef CONFIG_H
#define CONFIG_H

#include <stdint.h>

/* ===================== Wi-Fi Config ===================== */

/**
 * @brief Wi-Fi SSID
 */
#define WIFI_SSID       "P.202"

/**
 * @brief Wi-Fi password
 */
#define WIFI_PASS       "@123456789"

/**
 * @brief Maximum Wi-Fi retry attempts
 */
#define NET_MAX_RETRY   5

/* ===================== AWS IoT Config ===================== */

/**
 * @brief Maximum MQTT retry attempts
 */
#define AWS_FAIL_MAX    3
/* ===================== AWS IoT Config ===================== */

/**
 * @brief AWS IoT endpoint
 * Format: "xxxxxx-ats.iot.<region>.amazonaws.com"
 */
#define AWS_IOT_HOST    "your-aws-iot-endpoint.amazonaws.com"

/**
 * @brief AWS IoT MQTT port (TLS)
 */
#define AWS_IOT_PORT    8883

/**
 * @brief Path to AWS root CA certificate
 */
#define AWS_ROOT_CA_PATH    "/spiffs/AmazonRootCA1.pem"

/**
 * @brief Path to device certificate
 */
#define AWS_CERT_PATH       "/spiffs/device_cert.pem"

/**
 * @brief Path to private key
 */
#define AWS_PRIVATE_KEY_PATH "/spiffs/private_key.pem"

/**
 * @brief AWS MQTT publish topic
 */
#define AWS_PUB_TOPIC   "esp32/test/pub"

/**
 * @brief AWS MQTT subscribe topic
 */
#define AWS_SUB_TOPIC   "esp32/test/sub"

/* ===================== Network Manager ===================== */

/**
 * @brief Periodic publish interval (ms)
 */
#define NET_MANAGER_PUBLISH_INTERVAL_MS   10000

/* ===================== 4G / LTE ===================== */
/**
 * @brief Placeholder for 4G config (APN, PIN, module UART pins)
 */
#define LTE_APN         "your.apn.here"
#define LTE_PIN         "1234"
#define LTE_UART_PORT   "/dev/uart1"

/* ===================== General ===================== */

/**
 * @brief Maximum payload size for MQTT publish
 */
#define MQTT_MAX_PAYLOAD_LEN   256

#endif // CONFIG_H
