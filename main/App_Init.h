#ifndef _APP_INIT_H_
#define _APP_INIT_H_
#include "driver/gpio.h"
#include "driver/ledc.h"
#include "driver/uart.h"

#include "esp_log.h"
#include "esp_err.h"
#include <stdio.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "freertos/event_groups.h"

#include "INMP441.h"
#include "MAX98367A.h"
#include "wifi_sta.h"
#include "wss_client.h"

//? ==================== 应用硬件配置 ====================

//? LED状态指示灯GPIO
#ifndef GPIO_OUTPUT_IO_0
#define GPIO_OUTPUT_IO_0    10
#endif

//? ==================== 应用任务配置 ====================

//? 音频处理任务优先级（最高优先级，保证实时性）
#ifndef TASK_AUDIO_PRIORITY
#define TASK_AUDIO_PRIORITY     10
#endif

//? LED指示任务优先级
#ifndef TASK_LED_PRIORITY
#define TASK_LED_PRIORITY       5
#endif

//? 任务堆栈大小（字节）
#ifndef TASK_AUDIO_STACK_SIZE
#define TASK_AUDIO_STACK_SIZE   4096
#endif

#ifndef TASK_LED_STACK_SIZE
#define TASK_LED_STACK_SIZE     2048
#endif

#define TAG "app_init"

extern QueueHandle_t audio_data_queue;      // 发送音频数据到 WebSocket
extern QueueHandle_t audio_playback_queue;  // 从 WebSocket 接收音频数据并播放


void app_init(void);
void gpio_Init(void);
void wifi_Init(void);
void on_ws_message(const char *msg, size_t len);
void wss_Init(void);



#endif