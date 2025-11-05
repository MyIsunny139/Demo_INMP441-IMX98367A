#include "INMP441.h"

i2s_chan_handle_t rx_handle;
void i2s_rx_init(void)
{
    i2s_chan_config_t chan_cfg = I2S_CHANNEL_DEFAULT_CONFIG(I2S_NUM_0, I2S_ROLE_MASTER);
    
    //dma frame num使用最大值，增大dma一次搬运的数据量，能够提高效率，减小杂音，使用1023可以做到没有一丝杂音
    chan_cfg.dma_frame_num = 511;
    i2s_new_channel(&chan_cfg, NULL, &rx_handle);
 
    i2s_std_config_t std_cfg = {
        .clk_cfg = I2S_STD_CLK_DEFAULT_CONFIG(SAMPLE_RATE),
        
        //虽然inmp441采集数据为24bit，但是仍可使用32bit来接收，中间存储过程不需考虑，只要让声音怎么进来就怎么出去即可
        .slot_cfg = I2S_STD_MSB_SLOT_DEFAULT_CONFIG(I2S_DATA_BIT_WIDTH_32BIT, I2S_SLOT_MODE_STEREO),
        .gpio_cfg = {
            .mclk = I2S_GPIO_UNUSED,
            .dout = I2S_GPIO_UNUSED,
            .bclk = INMP_SCK,
            .ws = INMP_WS,
            .din = INMP_SD,
            .invert_flags = {
                .mclk_inv = false,
                .bclk_inv = false,
                .ws_inv = false,
            },
        },
    };
 
    i2s_channel_init_std_mode(rx_handle, &std_cfg);
 
    i2s_channel_enable(rx_handle);
}