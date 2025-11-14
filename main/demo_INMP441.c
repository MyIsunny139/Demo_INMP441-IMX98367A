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
#include "INMP441.h"
#include "MAX98367A.h"

// 外部声明音频数据队列
extern QueueHandle_t audio_data_queue;
extern QueueHandle_t audio_playback_queue;

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

void i2s_read_task(void *pvParameters)
{
    size_t bytes_read = 0;
    size_t bytes_written = 0;
 
    //? 一次性读取buf_size数量的音频，即dma最大搬运一次的数据量
    //? 读取后检测音频能量，如果有有效声音才应用增益并发送
    while (1) 
    {
        if (i2s_channel_read(rx_handle, buf, BUF_SIZE, &bytes_read, 100) == ESP_OK)
        {
            //? 1. 检测音频能量（判断是否有有效声音）
            int32_t *samples = (int32_t *)buf;
            size_t sample_count = bytes_read / sizeof(int32_t);
            int64_t total_energy = 0;
            
            //? 计算音频能量（所有采样点的绝对值之和）
            for (size_t i = 0; i < sample_count; i++) {
                total_energy += abs(samples[i]);
            }
            
            //? 计算平均能量
            int64_t avg_energy = total_energy / sample_count;
            
            //? 只有当平均能量超过阈值时才处理和发送数据
            //? 阈值设置：50000 表示静音或极小声音时不发送
            const int64_t AUDIO_ENERGY_THRESHOLD = 50000;
            
            if (avg_energy > AUDIO_ENERGY_THRESHOLD) {
                //? 2. 应用音量增益（只对有效音频增益）
                max98367a_apply_gain(buf, bytes_read);
                // for(int i=0;i<bytes_read;i++)
                // {
                //     printf("%d ",buf[i]);
                // }
                // printf("\r\n");

                //? 3. 发送完整音频数据到队列（2048字节完整DMA帧）
                if (audio_data_queue != NULL) {
                    //? 非阻塞方式发送完整缓冲区

                    if (xQueueSend(audio_data_queue, buf, 0) != pdPASS) {
                        //? 队列满，丢弃数据（优先保证实时性）
                        // ESP_LOGW("AUDIO", "Queue full, dropping audio frame");
                    }
                }
            }
            // else {
            //     //? 音频能量过低，不发送数据
            //     // ESP_LOGD("AUDIO", "Audio energy too low: %lld, skipping", avg_energy);
            // }
        }
        //? 移除延迟，让任务以最快速度运行，提高音频实时性
         vTaskDelay(pdMS_TO_TICKS(5)); 
    }
    vTaskDelete(NULL);
}

void i2s_send_task(void *pvParameters)
{
    uint8_t playback_buf[BUF_SIZE] = {0};  // 播放缓冲区（2048字节完整DMA帧）
    size_t bytes_written = 0;

    while (1) 
    {
        //? 从播放队列获取音频数据
        if (audio_playback_queue != NULL) {
            //? 等待从队列接收完整音频帧，超时100ms
            if (xQueueReceive(audio_playback_queue, playback_buf, pdMS_TO_TICKS(100)) == pdTRUE) {
                //? 播放接收到的完整音频数据（2048字节）
                esp_err_t ret = i2s_channel_write(tx_handle, playback_buf, BUF_SIZE, &bytes_written, portMAX_DELAY);
                if (ret != ESP_OK) {
                    ESP_LOGE("I2S_SEND", "Failed to write audio data: %s", esp_err_to_name(ret));
                } else if (bytes_written != BUF_SIZE) {
                    ESP_LOGW("I2S_SEND", "Incomplete write: %d/%d bytes", bytes_written, BUF_SIZE);
                }
            }
        }
        else {
            //? 队列未创建，等待
            vTaskDelay(pdMS_TO_TICKS(100));
        }
    }
    vTaskDelete(NULL);
}


void app_main(void)
{
    app_init();

    // 创建音频数据队列（麦克风 → WebSocket）
    // 队列元素大小改为 2048 字节（完整 DMA 帧），深度减少到 5
    audio_data_queue = xQueueCreate(5, sizeof(uint8_t) * BUF_SIZE);
    if (audio_data_queue == NULL) {
        ESP_LOGE("MAIN", "Failed to create audio_data_queue");
    }
    
    // 创建音频播放队列（WebSocket → 扬声器）
    audio_playback_queue = xQueueCreate(5, sizeof(uint8_t) * BUF_SIZE);
    if (audio_playback_queue == NULL) {
        ESP_LOGE("MAIN", "Failed to create audio_playback_queue");
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
    BaseType_t ret2 = xTaskCreatePinnedToCore(i2s_read_task, "i2s_read_send_task", TASK_AUDIO_STACK_SIZE, NULL, TASK_AUDIO_PRIORITY, NULL, 0);
    if (ret2 != pdPASS) 
    {
        ESP_LOGE("MAIN", "Failed to create i2s_read_send_task");
    }
    else 
    {
        ESP_LOGI("MAIN", "i2s_read_send_task created successfully");
    }
    BaseType_t ret3 = xTaskCreatePinnedToCore(i2s_send_task, "i2s_send_task", TASK_AUDIO_STACK_SIZE, NULL, TASK_AUDIO_PRIORITY, NULL, 0);
    if (ret2 != pdPASS) 
    {
        ESP_LOGE("MAIN", "Failed to create i2s_read_send_task");
    }
    else 
    {
        ESP_LOGI("MAIN", "i2s_read_send_task created successfully");
    }
}