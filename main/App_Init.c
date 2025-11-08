#include "App_Init.h"

QueueHandle_t audio_data_queue = NULL;    // Queue for audio data transmission

void app_init()
{
    i2s_rx_init();
    i2s_tx_init();
    gpio_Init();
    
    wifi_Init();
    
    //? 等待WiFi连接成功
    ESP_LOGI(TAG, "Waiting for WiFi connection...");
    int wait_count = 0;
    while (!wifi_sta_is_connected() && wait_count < 30)
    {
        vTaskDelay(1000 / portTICK_PERIOD_MS);
        wait_count++;
        ESP_LOGI(TAG, "WiFi waiting... %d/30", wait_count);
    }
    
    if (wifi_sta_is_connected())
    {
        ESP_LOGI(TAG, "WiFi connected successfully!");
        //? 额外等待2秒确保网络稳定
        vTaskDelay(2000 / portTICK_PERIOD_MS);
        wss_Init();     // 初始化WebSocket客户端
    }
    else
    {
        ESP_LOGE(TAG, "WiFi connection timeout, WebSocket will not start");
    }
}

void gpio_Init(void)
{
    gpio_config_t led_cfg = {
        .pin_bit_mask = 1ULL << GPIO_OUTPUT_IO_0,
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = 0,
        .pull_down_en = 0,
        .intr_type = GPIO_INTR_DISABLE
    };
    gpio_config(&led_cfg);
}

void wifi_Init(void)
{
    //? 使用独立的WiFi STA组件
    wifi_sta_config wifi_config = {
        .ssid = WIFI_SSID,
        .password = WIFI_PASS,
        .max_retry = WIFI_MAX_RETRY
    };
    
    esp_err_t ret = wifi_sta_init(&wifi_config);
    if (ret != ESP_OK)
    {
        ESP_LOGE(TAG, "WiFi initialization failed: %s", esp_err_to_name(ret));
    }
}

void on_ws_message(const char *msg, size_t len) 
{
    ESP_LOGI(TAG, "Received WebSocket message: %.*s", (int)len, msg);
}

// 保持WebSocket配置在全局范围内
static const wss_client_config_t ws_cfg = {
    .uri = WSS_URI,  // 使用统一配置的WebSocket服务器地址
    .on_message = on_ws_message,
};

void wss_Init(void)
{
    // 延迟启动WebSocket客户端，确保WiFi已连接
    wss_client_start(&ws_cfg);
}
