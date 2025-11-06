#include "App_Init.h"

QueueHandle_t ledc_queue = NULL;    // Queue for LEDC events
EventGroupHandle_t ledc_event_handle = NULL;    // Event group for LEDC events
QueueHandle_t audio_data_queue = NULL;    // Queue for audio data transmission

void app_init()
{

    i2s_rx_init();
    i2s_tx_init();
    gpio_Init();
    ledc_Init();
    wifi_Init();
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

bool IRAM_ATTR ledc_cb(const ledc_cb_param_t *param, void *user_arg)    
{
    // Check if the event is a full event
    BaseType_t taskWoken = pdFALSE;
    uint32_t duty = param->duty;
    /* Send duty value to queue from ISR. xQueueSendFromISR expects pointer to item. */
    xQueueSendFromISR(ledc_queue, &duty, &taskWoken);

    /* Set event bits if duty is full or empty. Use the same taskWoken to aggregate wake-up flags. */
    if (duty == 4095) {
        xEventGroupSetBitsFromISR(ledc_event_handle, FULL_EVENT_BIT0, &taskWoken);
    } else if (duty == 0) {
        xEventGroupSetBitsFromISR(ledc_event_handle, EMPTY_EVENT_BIT1, &taskWoken);
    }
    portYIELD_FROM_ISR(taskWoken);    // Yield from ISR if a task was woken
    return true;    // Return true to indicate successful handling
}   

void ledc_Init(void)
{
    ledc_timer_config_t ledc_timer = {
        .duty_resolution = LEDC_TIMER_12_BIT, // resolution of PWM duty 占空比分辨率2^13-1
        .freq_hz = 5000,                      // frequency of PWM signal pwm频率5kHz
        .speed_mode = LEDC_LOW_SPEED_MODE,   // timer mode
        .timer_num = LEDC_TIMER_0,            // timer index
        .clk_cfg = LEDC_AUTO_CLK,             // Auto select the source clock
    };
    ledc_timer_config(&ledc_timer);              // configure LED timer

    ledc_channel_config_t ledc_channel = {
        .channel = LEDC_CHANNEL_0,              // channel  通道
        .duty = 0,                              // duty     占空比  
        .gpio_num = GPIO_OUTPUT_IO_1,           // GPIO number          GPIO输出
        .speed_mode = LEDC_LOW_SPEED_MODE,      // timer mode           定时器模式
        .timer_sel = LEDC_TIMER_0,               // timer index          定时器索引
        .intr_type = LEDC_INTR_DISABLE,           // interrupt type       中断类型
    };
    ledc_channel_config(&ledc_channel);


    ledc_event_handle = xEventGroupCreate(); //创建事件组
    if (ledc_event_handle == NULL) {
        ESP_LOGE("LEDC", "Failed to create event group");
        return;
    }
    
    ledc_queue = xQueueCreate(10, sizeof(uint32_t)); // 创建队列用于从 ISR 传递 duty 值
    if (ledc_queue == NULL) {
        ESP_LOGE("LEDC", "Failed to create ledc_queue");
        return;
    }
    
    ledc_fade_func_install(0); //安装渐变函数
    
    // 先注册回调函数，再启动渐变
    ledc_cbs_t cbs = {
        .fade_cb = ledc_cb,
    };
    esp_err_t ret = ledc_cb_register(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0, &cbs, NULL);    //注册回调函数
    if (ret != ESP_OK) {
        ESP_LOGE("LEDC", "Failed to register callback: %s", esp_err_to_name(ret));
        return;
    }
    
    ledc_set_fade_with_time(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0, 4095, 1000); //设置渐变参数
    ledc_fade_start(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0, LEDC_FADE_NO_WAIT); //启动渐变
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


