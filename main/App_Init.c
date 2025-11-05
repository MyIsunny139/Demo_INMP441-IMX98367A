#include "App_Init.h"

QueueHandle_t ledc_queue = NULL;    // Queue for LEDC events
EventGroupHandle_t ledc_event_handle = NULL;    // Event group for LEDC events
QueueHandle_t audio_data_queue = NULL;    // Queue for audio data transmission

void app_init()
{
    gpio_Init();
    ledc_Init();
    
    // 创建音频数据队列，每个元素256个采样点
    audio_data_queue = xQueueCreate(5, sizeof(int16_t) * 256);
    if (audio_data_queue == NULL) {
        ESP_LOGE("APP_INIT", "Failed to create audio data queue");
    }
    
    INMP441_Init();    // 初始化INMP441麦克风模块
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

