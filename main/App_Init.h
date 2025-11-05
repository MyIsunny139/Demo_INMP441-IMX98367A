#ifndef _APP_INIT_H_
#define _APP_INIT_H_
#include "driver/gpio.h"
#include "driver/ledc.h"
#include "driver/uart.h"
#include "esp_log.h"
#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "freertos/event_groups.h"
#include "INMP441.h"

#define GPIO_OUTPUT_IO_0    GPIO_NUM_10
#define GPIO_OUTPUT_IO_1    GPIO_NUM_11
#define FULL_EVENT_BIT0    BIT0    // Bit for full event
#define EMPTY_EVENT_BIT1    BIT1    // Bit for empty event


extern QueueHandle_t ledc_queue;    // Queue for LEDC events
extern EventGroupHandle_t ledc_event_handle;    // Event group for LEDC events
extern QueueHandle_t audio_data_queue;    // Queue for audio data transmission


void app_init(void);
void gpio_Init(void);

void ledc_Init(void);

void i2s_Init(void);


#endif