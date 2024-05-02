#include "Input.h"
static const char* TAG_IN = "input";


i2s_chan_handle_t i2s_in_init() 
{
    ESP_LOGE(TAG_IN, "Creating Microphone");
    i2s_chan_handle_t handle;

    i2s_chan_config_t chan_cfg = I2S_CHANNEL_DEFAULT_CONFIG(I2S_NUM_AUTO, I2S_ROLE_MASTER);

    ESP_ERROR_CHECK(i2s_new_channel(&chan_cfg, NULL, &handle));

    i2s_std_config_t std_cfg = {
    .clk_cfg =  I2S_STD_CLK_DEFAULT_CONFIG(16000),
    .slot_cfg = I2S_STD_PHILIPS_SLOT_DEFAULT_CONFIG(I2S_DATA_BIT_WIDTH_32BIT,  I2S_SLOT_MODE_MONO),
    .gpio_cfg = {
        .mclk = I2S_GPIO_UNUSED,
        .bclk = GPIO_NUM_5,
        .ws = GPIO_NUM_4,
        .dout = I2S_GPIO_UNUSED,
        .din = GPIO_NUM_6,
        .invert_flags = {
            .mclk_inv = false,
            .bclk_inv = false,
            .ws_inv = false,
            },
        },
    };
    std_cfg.slot_cfg.slot_mask = I2S_STD_SLOT_RIGHT;
    i2s_channel_init_std_mode(handle, &std_cfg);
    ESP_LOGE(TAG_IN, "Microphone created");
    return handle;
};

void start_in(i2s_chan_handle_t * handle) 
{
    i2s_channel_enable(*handle);
}

int read_i2s(i2s_chan_handle_t * handle, int16_t* samples, int count) 
{
    int32_t *raw_samples = (int32_t *)malloc(sizeof(int32_t) * count);
    size_t bytes_read = 0;
    if  (i2s_channel_read(*handle, raw_samples, sizeof(int32_t) * count, &bytes_read, portMAX_DELAY) == ESP_OK )
    {
        int samples_read = bytes_read / sizeof(int32_t);
        for (int i = 0; i < samples_read; i++)
        {
        samples[i] = (raw_samples[i] & 0xFFFFFFF0) >> 11;
   
        }
        free(raw_samples);
        return samples_read;
    } else {
        free(raw_samples);
        return 0;
    }
}

void stop_in(i2s_chan_handle_t * handle) 
{
    i2s_channel_disable(*handle);
}

void delete_channel(i2s_chan_handle_t * handle) 
{
    i2s_del_channel(*handle);
}
