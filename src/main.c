#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "driver/rmt.h"
#include "freertos/task.h"
#include "network.h"
#include "aws_mqtt_client.h"
#include "network_manager.h"
#include "esp_log.h"
#include "esp_wifi.h"

#define LED_PIN GPIO_NUM_48
#define RMT_CHANNEL RMT_CHANNEL_0
#define NUM_LEDS 1
#define TAG "ESP32_MAC"

/**
 * @brief In ra MAC address của ESP32-S3
 */
void print_mac_addresses(void)
{
    uint8_t mac[6];

    // Lấy MAC của WiFi Station (STA)
    if (esp_wifi_get_mac(WIFI_IF_STA, mac) == ESP_OK) {
        ESP_LOGI(TAG, "STA MAC: %02X:%02X:%02X:%02X:%02X:%02X",
                 mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
    } else {
        ESP_LOGE(TAG, "Failed to get STA MAC");
    }

    // Lấy MAC của WiFi Access Point (AP)
    if (esp_wifi_get_mac(WIFI_IF_AP, mac) == ESP_OK) {
        ESP_LOGI(TAG, "AP  MAC: %02X:%02X:%02X:%02X:%02X:%02X",
                 mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
    } else {
        ESP_LOGE(TAG, "Failed to get AP MAC");
    }
}


void app_main() {

    esp_log_level_set("*", ESP_LOG_INFO);
    ESP_LOGI("app", "Starting network manager demo");

    // start network manager task
    network_manager_start();

    // Example: request manual publish after 20s
    vTaskDelay(pdMS_TO_TICKS(20000));
    network_manager_request_publish("{\"manual\":\"publish\"}");
    print_mac_addresses();
}