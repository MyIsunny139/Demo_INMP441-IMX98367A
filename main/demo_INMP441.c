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

uint8_t buf[BUF_SIZE];
void i2s_read_send_task(void *pvParameters)
{
    size_t bytes = 0;
 
    //一次性读取buf_size数量的音频，即dma最大搬运一次的数据量，读成功后，写入tx，即可通过MAX98357A播放
    while (1) {
        if (i2s_channel_read(rx_handle, buf, BUF_SIZE, &bytes, 1000) == ESP_OK)
        {
            i2s_channel_write(tx_handle, buf, BUF_SIZE, &bytes, 1000);
        }
        vTaskDelay(pdMS_TO_TICKS(5));
    }
    vTaskDelete(NULL);

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




    BaseType_t ret2 = xTaskCreatePinnedToCore(i2s_read_send_task, "i2s_read_send_task", 4096, NULL, 9, NULL, 0);
    if (ret2 != pdPASS) {
        ESP_LOGE("MAIN", "Failed to create i2s_read_send_task");
    }else {
        ESP_LOGI("MAIN", "i2s_read_send_task created successfully");
    }
}




