#include "MAX98367A.h"

i2s_chan_handle_t tx_handle;


void i2s_tx_init(void)
{
    i2s_chan_config_t chan_cfg = I2S_CHANNEL_DEFAULT_CONFIG(I2S_NUM_1, I2S_ROLE_MASTER);
    
    //? 优化：减小dma frame num，降低延迟，提高实时性
    chan_cfg.dma_frame_num = MAX98367A_DMA_FRAME_NUM;
    chan_cfg.auto_clear = true;     //? 自动清除DMA缓冲区
    i2s_new_channel(&chan_cfg, &tx_handle, NULL);
 
    i2s_std_config_t std_cfg = {
        .clk_cfg = I2S_STD_CLK_DEFAULT_CONFIG(MAX98367A_SAMPLE_RATE),
        .slot_cfg = I2S_STD_MSB_SLOT_DEFAULT_CONFIG(I2S_DATA_BIT_WIDTH_32BIT, MAX98367A_CHANNEL_MODE),
        .gpio_cfg = {
            .mclk = I2S_GPIO_UNUSED,
            .din = I2S_GPIO_UNUSED,
            .bclk = MAX_BCLK,
            .ws = MAX_LRC,
            .dout = MAX_DIN,
            .invert_flags = {
                .mclk_inv = false,
                .bclk_inv = false,
                .ws_inv = false,
            },
        },
    };
 
    i2s_channel_init_std_mode(tx_handle, &std_cfg);
 
    i2s_channel_enable(tx_handle);
}