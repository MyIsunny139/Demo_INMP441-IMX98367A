#include "App_Init.h"

QueueHandle_t audio_data_queue = NULL;    // Queue for audio data transmission

void app_init()
{

    i2s_rx_init();
    i2s_tx_init();
    gpio_Init();
    //? 移除LEDC初始化，改为简单的LED闪烁
    // ledc_Init();
    
    wifi_Init();
    vTaskDelay(5000/portTICK_PERIOD_MS);    // Delay for 5 seconds
    
    //? 增加延迟，确保WiFi完全连接并获取IP
    vTaskDelay(3000/portTICK_PERIOD_MS);    // 额外等待3秒
    
    wss_Init();     // 初始化WebSocket客户端
    // 创建音频数据队列，每个元素256个采样点

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


void event_handle(void* event_handler_arg, esp_event_base_t event_base, int32_t event_id, void* event_data)
{
    if(event_base == WIFI_EVENT)
    {
        switch (event_id)
        {
        case WIFI_EVENT_STA_START:          //? STA工作模式已启动
            esp_wifi_connect();             //? 连接WiFi
            break;
        case WIFI_EVENT_STA_CONNECTED:      //? EPS32已经成功连接到路由器
            ESP_LOGI(TAG, "esp32 connected to api!");
            break;
        case WIFI_EVENT_STA_DISCONNECTED:   //? 断连
            esp_wifi_connect();
        default:
            break;
        }
    }
    else if(event_base == IP_EVENT)
    {
        switch (event_id)
        {
        case IP_EVENT_STA_GOT_IP:           //? EPS32获取到无线路由器分配的IP,此时ESP32才真正的连接到路由器（如果路由器能连接到互联网，ESP32也能连接到互联网）
            ESP_LOGI(TAG, "esp32 get ip address");
            break;
        default:
            break;
        }
    }
}




void wifi_Init(void)
{
     #ifndef MY_CFG_ORDER
	    /*  1.初始化NVS
	        默认状态下，当我们使用一组SSID和密码连接WiFi成功后，ESP-IDF的底层会帮我们把这组SSID和密码保存到NVS中;
	        下次系统重启后，当进入STA模式并进行连接时，就会用这组SSID和密码进行连接；
	    */ 
	    ESP_ERROR_CHECK(nvs_flash_init());      //? 宏ESP_ERROR_CHECK，用来检查NVS初始化是否有问题；
	    //  2.初始化TCP/IP协议栈（ESP-IDF中使用的时LWIP）
	    ESP_ERROR_CHECK(esp_netif_init());      //? netif--->net interface，网络接口的缩写；
	    /*  3.创建事件系统循环
	        WiFi连接过程中会产生各种的事件，这些事件都是通过回调函数来通知我们的
	    */
	    ESP_ERROR_CHECK(esp_event_loop_create_default());
	    //  4.创建STA
	    esp_netif_create_default_wifi_sta();    //? 该函数会返回一个STA网卡对象，这里我们使用不到，可以忽略该返回值；
	    /*  5.初始化WiFi
	        定义一个WiFi初始化结构体，将其设置到WiFi初始化函数中；
	        该步骤会设置WiFi的缓冲区数量，加密功能等，我们按默认功能设置就可以；
	    */
	    wifi_init_config_t wifi_init_cfg = WIFI_INIT_CONFIG_DEFAULT();
	    ESP_ERROR_CHECK(esp_wifi_init(&wifi_init_cfg));
	    //  6.注册事件响应
	    esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, event_handle, NULL);      //? 注册WiFi事件回调
	    esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, event_handle, NULL);     //? 注册IP事件回调(连接到路由器，获取到IP地址后就会触发该事件)
	    //  7.WiFi参数配置
	    wifi_config_t wifi_config = {
	        .sta = {
	            .ssid = CON_SSID,                               //? SSID，服务器集标识符，即WiFi名称；
	            .password = CON_PASSWORD,                       //? WiFi密码；
	            .threshold.authmode = WIFI_AUTH_WPA2_PSK,       //? 配置加密模式，目前常用的时WPA2_PSK;
	            .pmf_cfg.capable = true,                        //? 是否启用保护管理帧，启用后可以增强安全性；
	            .pmf_cfg.required = false,                      //? 是否只和有保护管理帧功能的设备通信；
	        },
	    };
	    esp_wifi_set_config(WIFI_IF_STA, &wifi_config);
	    //  8.设置WiFi模式
	    esp_wifi_set_mode(WIFI_MODE_STA);
	    //  9.启动WiFi
	    esp_wifi_start();
	    
    #else
	    /*  1.初始化NVS
	        默认状态下，当我们使用一组SSID和密码连接WiFi成功后，ESP-IDF的底层会帮我们把这组SSID和密码保存到NVS中;
	        下次系统重启后，当进入STA模式并进行连接时，就会用这组SSID和密码进行连接；
	    */ 
	    ESP_ERROR_CHECK(nvs_flash_init());      //? 宏ESP_ERROR_CHECK，用来检查NVS初始化是否有问题；
	    //  2.初始化TCP/IP协议栈（ESP-IDF中使用的时LWIP）
	    ESP_ERROR_CHECK(esp_netif_init());      //? netif--->net interface，网络接口的缩写；
	    /*  3.创建事件系统循环
	        WiFi连接过程中会产生各种的事件，这些事件都是通过回调函数来通知我们的
	    */
	    ESP_ERROR_CHECK(esp_event_loop_create_default());
	    //  6.注册事件响应
	    esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, event_handle, NULL);      //? 注册WiFi事件回调
	    esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, event_handle, NULL);     //? 注册IP事件回调(连接到路由器，获取到IP地址后就会触发该事件)
	    //  4.创建STA
	    esp_netif_create_default_wifi_sta();    //? 该函数会返回一个STA网卡对象，这里我们使用不到，可以忽略该返回值；
	    //  8.设置WiFi模式为STA
	    esp_wifi_set_mode(WIFI_MODE_STA);
	    /*  5.初始化WiFi
	        定义一个WiFi初始化结构体，将其设置到WiFi初始化函数中；
	        该步骤会设置WiFi的缓冲区数量，加密功能等，我们按默认功能设置就可以；
	    */
	    wifi_init_config_t wifi_init_cfg = WIFI_INIT_CONFIG_DEFAULT();
	    ESP_ERROR_CHECK(esp_wifi_init(&wifi_init_cfg));
	    //  7.WiFi参数配置
	    wifi_config_t wifi_config = {
	        .sta = {
	            .ssid = CON_SSID,                               //? SSID，服务器集标识符，即WiFi名称；
	            .password = CON_PASSWORD,                       //? WiFi密码；
	            .threshold.authmode = WIFI_AUTH_WPA2_PSK,       //? 配置加密模式，目前常用的时WPA2_PSK;
	            .pmf_cfg.capable = true,                        //? 是否启用保护管理帧，启用后可以增强安全性；
	            .pmf_cfg.required = false,                      //? 是否只和有保护管理帧功能的设备通信；
	        },
	    };
	    esp_wifi_set_config(WIFI_IF_STA, &wifi_config);
	    //  9.启动WiFi
	    esp_wifi_start();
    #endif
}


void on_ws_message(const char *msg, size_t len) {
    ESP_LOGI(TAG, "Received WebSocket message: %.*s", (int)len, msg);
}


// 保持WebSocket配置在全局范围内
static const wss_client_config_t ws_cfg = {
    .uri = WEBSOCKET_SERVER_URI,  // 替换为你的WebSocket服务器地址
    .on_message = on_ws_message,
};

void wss_Init(void)
{
    // 延迟启动WebSocket客户端，确保WiFi已连接
    wss_client_start(&ws_cfg);
}

