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

// 外部声明音频数据队列
extern QueueHandle_t audio_data_queue;




void led_run_task(void *pvParameters)
{
    while(1) {
        uint32_t duty = 0;
        if(xQueueReceive(ledc_queue, &duty, portMAX_DELAY) == pdTRUE) {
            // printf("LEDC Queue Received Duty: %ld\n", duty);
        }

        if(xEventGroupWaitBits(ledc_event_handle, FULL_EVENT_BIT0, pdTRUE, pdFALSE, portMAX_DELAY) & FULL_EVENT_BIT0) {
            // printf("LEDC Full Event Detected: Duty Cycle is 4095\n");
            gpio_set_level(GPIO_OUTPUT_IO_0, 1); // Turn on LED
            ledc_set_fade_with_time(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0, 0, 2000); // Set fade to 0
            ledc_fade_start(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0, LEDC_FADE_NO_WAIT); // Start fade
            
        } 
        if(xEventGroupWaitBits(ledc_event_handle, EMPTY_EVENT_BIT1, pdTRUE, pdFALSE, portMAX_DELAY) & EMPTY_EVENT_BIT1) {
            // printf("LEDC Empty Event Detected: Duty Cycle is 0\n");
            gpio_set_level(GPIO_OUTPUT_IO_0, 0); // Turn off LED
            ledc_set_fade_with_time(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0, 4095, 2000); // Set fade to 8191
            ledc_fade_start(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0, LEDC_FADE_NO_WAIT); // Start fade
            
        }
        /* 回调已在 app_main 中注册，避免在循环中重复注册 */
        vTaskDelay(pdMS_TO_TICKS(500)); // Delay for visibility
    }
    return;
}
void INMP441_read_task(void *pvParameters)
{
    #define AUDIO_BUFFER_SIZE 128
    int16_t audio_buffer[AUDIO_BUFFER_SIZE];
    size_t bytes_read = 0;
    
    while (1)
    {
        // 读取音频数据
        esp_err_t ret = INMP441_Read(audio_buffer, AUDIO_BUFFER_SIZE * sizeof(int16_t), &bytes_read);
        if (ret == ESP_OK) {
            // 计算采样点数并应用阈值滤波
            size_t samples_read = bytes_read / sizeof(int16_t);
            for (int i = 0; i < samples_read; i++) {
                if (abs(audio_buffer[i]) < 20) {
                    audio_buffer[i] = 0;
                }
            }
            
            // 发送滤波后的数据
            if (xQueueSend(audio_data_queue, audio_buffer, pdMS_TO_TICKS(20)) != pdPASS) {
                ESP_LOGW("INMP441", "Queue full, dropping audio packet");
            }
        } else {
            ESP_LOGE("INMP441", "Read failed: %s", esp_err_to_name(ret));
        }
        
        vTaskDelay(pdMS_TO_TICKS(25));
    }
    
    return;
}
void audio_send_task(void *pvParameters)
{
    int16_t audio_buffer[128];  // 与读取任务缓冲区大小一致
    
    while (1) {
        // 从队列接收数据
        if (xQueueReceive(audio_data_queue, audio_buffer, portMAX_DELAY) == pdPASS) {
            // 精简数据输出格式
            // printf("Audio[%lu]:", (unsigned long)xTaskGetTickCount());
            for (int i = 0; i < 64; i++) {  // 只打印前10个样本作为示例
                printf("video=%d\r\n", audio_buffer[i]);
            }
        }
        
        vTaskDelay(pdMS_TO_TICKS(25));  // 与读取任务保持同步
    }
}

void app_main(void)
{
    app_init();

    
    BaseType_t ret1 = xTaskCreatePinnedToCore(led_run_task, "led_run_task", 2048, NULL, 10, NULL, 0);
    if (ret1 != pdPASS) {
        ESP_LOGE("MAIN", "Failed to create led_run_task");
    } else {
        ESP_LOGI("MAIN", "led_run_task created successfully");
    }
    
    BaseType_t ret2 = xTaskCreatePinnedToCore(INMP441_read_task, "INMP441_read_task", 4096, NULL, 10, NULL, 1);
    if (ret2 != pdPASS) {
        ESP_LOGE("MAIN", "Failed to create INMP441_read_task");
    } else {
        ESP_LOGI("MAIN", "INMP441_read_task created successfully");
    }

    // 创建音频数据发送任务
    BaseType_t ret3 = xTaskCreatePinnedToCore(audio_send_task, "audio_send_task", 4096, NULL, 9, NULL, 1);
    if (ret3 != pdPASS) {
        ESP_LOGE("MAIN", "Failed to create audio_send_task");
    } else {
        ESP_LOGI("MAIN", "audio_send_task created successfully");
    }
}




