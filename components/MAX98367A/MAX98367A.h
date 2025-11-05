#ifndef _MAX98367A_H_
#define _MAX98367A_H_


#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/i2s_std.h"
#include "driver/gpio.h"
#include "INMP441.h"
//MAX98357A引脚，根据自己连线修改
#define MAX_DIN     GPIO_NUM_8
#define MAX_BCLK    GPIO_NUM_3
#define MAX_LRC     GPIO_NUM_46

//配置rx对INMP441的采样率为44.1kHz，这是常用的人声采样率
#define SAMPLE_RATE 44100
//buf size计算方法：根据esp32官方文档，buf size = dma frame num * 声道数 * 数据位宽 / 8
#define BUF_SIZE    (1023 * 1 * 32 / 8)

extern i2s_chan_handle_t tx_handle;



void i2s_tx_init(void);

#endif