#ifndef _INMP441_H_
#define _INMP441_H_
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/i2s.h"
#include "driver/gpio.h"
#include "esp_err.h"
#include "esp_log.h"

// I2S引脚配置
#define I2S_MIC_WS      GPIO_NUM_40  // 串行时钟线（WS）
#define I2S_MIC_SD      GPIO_NUM_41  // 串行数据线（SD）
#define I2S_MIC_SCK     GPIO_NUM_42  // 串行时钟（SCK）
#define I2S_PORT        I2S_NUM_0

// 音频配置
#define SAMPLE_RATE     44100
#define BUFFER_LEN      1024
#define BITS_PER_SAMPLE 16

/**
 * @brief 初始化INMP441麦克风
 * @return esp_err_t ESP_OK表示成功，其他表示错误
 */
esp_err_t INMP441_Init(void);

/**
 * @brief 读取音频数据
 * @param buffer 存储音频数据的缓冲区
 * @param buffer_size 缓冲区大小
 * @param bytes_read 实际读取的字节数
 * @return esp_err_t ESP_OK表示成功，其他表示错误
 */
esp_err_t INMP441_Read(int16_t *buffer, size_t buffer_size, size_t *bytes_read);

/**
 * @brief 停止I2S并释放资源
 */
void INMP441_Deinit(void);

#endif