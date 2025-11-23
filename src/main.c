#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "driver/rmt.h"
#include "freertos/task.h"
#include "network.h"
#include "aws_mqtt_client.h"
#include "esp_log.h"

#define LED_PIN GPIO_NUM_48
#define RMT_CHANNEL RMT_CHANNEL_0
#define NUM_LEDS 1

// Mỗi màu là 24 bit (GRB)
static void ws2812_write_color(uint8_t green, uint8_t red, uint8_t blue)
{
    rmt_item32_t items[24];
    uint32_t color = ((uint32_t)green << 16) | ((uint32_t)red << 8) | blue;

    for (int i = 0; i < 24; i++)
    {
        if (color & (1 << (23 - i)))
        {
            // 1: T=0.8us High, 0.45us Low
            items[i].duration0 = 8;   // 0.8us * 10
            items[i].level0 = 1;
            items[i].duration1 = 4;   // 0.45us * 10
            items[i].level1 = 0;
        }
        else
        {
            // 0: T=0.4us High, 0.85us Low
            items[i].duration0 = 4;  
            items[i].level0 = 1;
            items[i].duration1 = 8;  
            items[i].level1 = 0;
        }
    }
    rmt_write_items(RMT_CHANNEL, items, 24, true);
    rmt_wait_tx_done(RMT_CHANNEL, portMAX_DELAY);
}

void config_RMT_TX()
{
    // config RMT TX
    rmt_config_t config = RMT_DEFAULT_CONFIG_TX(LED_PIN, RMT_CHANNEL);
    config.clk_div = 2;  // tăng độ chính xác
    rmt_config(&config);
    rmt_driver_install(config.channel, 0, 0);
}

void set_rgb();

void app_main() {
    //config_RMT_TX();
    // while(1)
    // {
    //     ws2812_write_color(0, 0, 0);   // đỏ
    //     vTaskDelay(pdMS_TO_TICKS(1000));
    //     ws2812_write_color(0, 0, 0);   // xanh lá
    //     vTaskDelay(pdMS_TO_TICKS(1000));
    //     ws2812_write_color(0, 0, 0);   // xanh dương
    //     vTaskDelay(pdMS_TO_TICKS(1000));
    // }

    net_init(NET_IF_WIFI);
    net_connect(AWS_HOST, AWS_PORT);

    aws_mqtt_init();
    aws_mqtt_connect();
    aws_mqtt_publish("esp32s3/test", "{\"hello\":\"aws-iot\"}");
}