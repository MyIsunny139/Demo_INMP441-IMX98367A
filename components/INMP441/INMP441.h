#ifndef _INMP441_H_
#define _INMP441_H_
#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/i2s_std.h"
#include "driver/gpio.h"

//? INMP441引脚配置，根据自己连线修改
//? 注意：如需修改引脚配置，请直接修改此文件
#define INMP_SD     GPIO_NUM_5
#define INMP_SCK    GPIO_NUM_6
#define INMP_WS     GPIO_NUM_4

//? I2S配置参数
//? 注意：音频配置在此组件头文件中管理
#define INMP441_SAMPLE_RATE     44100   //? 采样率
#define INMP441_DMA_FRAME_NUM   256     //? DMA缓冲帧数
#define INMP441_BIT_WIDTH       32      //? 位宽
#define INMP441_CHANNEL_MODE    I2S_SLOT_MODE_STEREO  //? 声道模式

extern i2s_chan_handle_t rx_handle;

void i2s_rx_init(void);

#endif