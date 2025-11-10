#include <stdio.h>
#include <stdlib.h>  // 添加abs函数需要的头文件
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "freertos/event_groups.h"
#include "driver/gpio.h"
#include "driver/ledc.h"
#include "esp_log.h"
#include "App_Init.h"
#include "MAX98367A.h"

// 外部声明音频数据队列
extern QueueHandle_t audio_data_queue;

// 将音频缓冲区移到全局范围，避免占用任务栈空间
uint8_t buf[BUF_SIZE];

void led_run_task(void *pvParameters)
{
    bool led_state = false;
    
    while(1) 
    {
        //? 切换LED状态
        led_state = !led_state;
        gpio_set_level(GPIO_OUTPUT_IO_0, led_state ? 1 : 0);
        
        //? LED闪烁，500ms间隔
        vTaskDelay(pdMS_TO_TICKS(500));
    }
    vTaskDelete(NULL);
}

void i2s_read_send_task(void *pvParameters)
{
    size_t bytes_read = 0;
    size_t bytes_written = 0;
 
    //? 一次性读取buf_size数量的音频，即dma最大搬运一次的数据量，读成功后，应用增益，然后写入tx，即可通过MAX98357A播放
    //? 优化：移除延迟，提高实时性；减少超时时间
    while (1) 
    {
        if (i2s_channel_read(rx_handle, buf, BUF_SIZE, &bytes_read, 100) == ESP_OK)
        {
            //? 应用音量增益
            max98367a_apply_gain(buf, bytes_read);
            
            i2s_channel_write(tx_handle, buf, bytes_read, &bytes_written, 100);
        }
        //? 移除延迟，让任务以最快速度运行，提高音频实时性
        // vTaskDelay(pdMS_TO_TICKS(5));  // 已移除
    }
    vTaskDelete(NULL);
}


void app_main(void)
{
    app_init();

    //? 设置音量增益 (0.0~2.0, 默认1.0为原音量)
    //? 可以根据需要调整：0.5=减半, 1.5=1.5倍, 2.0=2倍
    max98367a_set_gain(2.0f);

    // 初始化音频数据队列
    audio_data_queue = xQueueCreate(10, sizeof(uint8_t) * 256);
    if (audio_data_queue == NULL) {
        ESP_LOGE("MAIN", "Failed to create audio_data_queue");
    }

    //? 创建LED状态指示任务
    BaseType_t ret1 = xTaskCreatePinnedToCore(led_run_task, "led_run_task", TASK_LED_STACK_SIZE, NULL, TASK_LED_PRIORITY, NULL, 0);
    if (ret1 != pdPASS) 
    {
        ESP_LOGE("MAIN", "Failed to create led_run_task");
    }
    else 
    {
        ESP_LOGI("MAIN", "led_run_task created successfully");
    }

    //? 创建I2S音频任务（最高优先级，确保实时性）
    BaseType_t ret2 = xTaskCreatePinnedToCore(i2s_read_send_task, "i2s_read_send_task", TASK_AUDIO_STACK_SIZE, NULL, TASK_AUDIO_PRIORITY, NULL, 0);
    if (ret2 != pdPASS) 
    {
        ESP_LOGE("MAIN", "Failed to create i2s_read_send_task");
    }
    else 
    {
        ESP_LOGI("MAIN", "i2s_read_send_task created successfully");
    }
}