#ifndef _MAX98367A_H_
#define _MAX98367A_H_


#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/i2s_std.h"
#include "driver/gpio.h"

//? MAX98357A引脚，根据自己连线修改
//? 注意：如需修改引脚配置，请直接修改此文件
#define MAX_DIN     GPIO_NUM_8
#define MAX_BCLK    GPIO_NUM_3
#define MAX_LRC     GPIO_NUM_46

//? I2S配置参数
//? 注意：音频配置在此组件头文件中管理
#define MAX98367A_SAMPLE_RATE     44100                 //? 采样率
#define MAX98367A_DMA_FRAME_NUM   256                   //? DMA缓冲帧数
#define MAX98367A_BIT_WIDTH       32                    //? 位宽
#define MAX98367A_CHANNEL_NUM     2                     //? 声道数
#define MAX98367A_CHANNEL_MODE    I2S_SLOT_MODE_STEREO  //? 声道模式

//? buf size计算方法：根据esp32官方文档，buf size = dma frame num * 声道数 * 数据位宽 / 8
//? 优化：减小缓冲区，降低延迟，提高实时性（从511改为256帧）
#define BUF_SIZE    (MAX98367A_DMA_FRAME_NUM * MAX98367A_CHANNEL_NUM * MAX98367A_BIT_WIDTH / 8)
#define SAMPLE_RATE MAX98367A_SAMPLE_RATE  //? 保留旧定义用于兼容

extern i2s_chan_handle_t tx_handle;



void i2s_tx_init(void);

#endif