#include "INMP441.h"


static const char *TAG = "INMP441";

esp_err_t INMP441_Init(void)
{
    esp_err_t ret = ESP_OK;
    
    // I2S驱动器配置
    i2s_config_t i2s_config = 
    {
        .mode = I2S_MODE_MASTER | I2S_MODE_RX,                  // 主模式，接收
        .sample_rate = SAMPLE_RATE,                             // 采样率
        .bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT,           // 16位采样
        .channel_format = I2S_CHANNEL_FMT_ONLY_LEFT,            // 单声道（左声道）
        .communication_format = I2S_COMM_FORMAT_STAND_I2S,     // 标准I2S格式
        .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1,               // 中断优先级
        .dma_buf_count = 8,                                     // DMA缓冲区数量
        .dma_buf_len = BUFFER_LEN,                              // 每个缓冲区长度
        .use_apll = false,                                      // 不使用APLL
        .tx_desc_auto_clear = false,                            // 不自动清除发送描述符
        .fixed_mclk = 0                                         // 固定MCLK频率
    };

    // 安装I2S驱动器
    ret = i2s_driver_install(I2S_PORT, &i2s_config, 0, NULL);
    if (ret != ESP_OK) 
    {
        ESP_LOGE(TAG, "I2S driver installation failed: %s", esp_err_to_name(ret));
        return ret;
    }

    // I2S引脚配置
    i2s_pin_config_t pin_config = 
    {
        .bck_io_num = I2S_MIC_SCK,      // 位时钟
        .ws_io_num = I2S_MIC_WS,        // 字选择（左右声道时钟）
        .data_out_num = I2S_PIN_NO_CHANGE, // 不使用数据输出
        .data_in_num = I2S_MIC_SD        // 数据输入（麦克风数据）
    };

    // 设置I2S引脚
    ret = i2s_set_pin(I2S_PORT, &pin_config);
    if (ret != ESP_OK) 
    {
        ESP_LOGE(TAG, "I2S pin configuration failed: %s", esp_err_to_name(ret));
        i2s_driver_uninstall(I2S_PORT);
        return ret;
    }

    // 设置I2S时钟（确保采样率准确）
    ret = i2s_set_clk(I2S_PORT, SAMPLE_RATE, I2S_BITS_PER_SAMPLE_16BIT, I2S_CHANNEL_MONO);
    if (ret != ESP_OK) 
    {
        ESP_LOGE(TAG, "I2S clock configuration failed: %s", esp_err_to_name(ret));
        i2s_driver_uninstall(I2S_PORT);
        return ret;
    }

    // 添加100ms延迟确保麦克风稳定
    vTaskDelay(pdMS_TO_TICKS(100));
    
    // 详细日志输出配置信息
    ESP_LOGI(TAG, "INMP441 initialized successfully");
    ESP_LOGI(TAG, "Sample rate: %d Hz, Bits per sample: %d", SAMPLE_RATE, BITS_PER_SAMPLE);
    ESP_LOGI(TAG, "GPIO Config: SCK=%d, WS=%d, SD=%d", 
            I2S_MIC_SCK, I2S_MIC_WS, I2S_MIC_SD);
    ESP_LOGI(TAG, "I2S Mode: Master RX, Standard I2S format");
    
    return ESP_OK;
}

/**
 * @brief 从INMP441麦克风读取音频数据
 *
 * @param buffer 存储读取数据的缓冲区指针
 * @param buffer_size 缓冲区大小(字节数)
 * @param bytes_read 实际读取的字节数指针
 * 
 * @return 
 *     - ESP_OK 读取成功
 *     - ESP_ERR_INVALID_ARG 参数无效
 *     - 其他I2S错误码
 */
esp_err_t INMP441_Read(int16_t *buffer, size_t buffer_size, size_t *bytes_read)
{
    if (buffer == NULL || bytes_read == NULL) 
    {
        return ESP_ERR_INVALID_ARG;
    }

    // 从I2S读取数据
    esp_err_t ret = i2s_read(I2S_PORT, buffer, buffer_size, bytes_read, portMAX_DELAY);
    
    if (ret != ESP_OK) 
    {
        ESP_LOGE(TAG, "I2S read failed: %s", esp_err_to_name(ret));
        return ret;
    }

    return ESP_OK;
}

void INMP441_Deinit(void)
{
    // 卸载I2S驱动器
    i2s_driver_uninstall(I2S_PORT);
    ESP_LOGI(TAG, "INMP441 deinitialized");
}