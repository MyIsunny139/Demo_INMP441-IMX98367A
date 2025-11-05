#ifndef _INMP441_H_
#define _INMP441_H_
#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/i2s_std.h"
#include "driver/gpio.h"
#include "MAX98367A.h"
//INMP引脚，根据自己连线修改
#define INMP_SD     GPIO_NUM_5
#define INMP_SCK    GPIO_NUM_6
#define INMP_WS     GPIO_NUM_4

extern i2s_chan_handle_t rx_handle;

void i2s_rx_init(void);

#endif