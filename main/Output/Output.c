#include "Output.h"


i2s_chan_handle_t i2s_out_init() 
{
    
    i2s_chan_handle_t handle;    
    i2s_chan_config_t chan_cfg = I2S_CHANNEL_DEFAULT_CONFIG(I2S_NUM_AUTO, I2S_ROLE_MASTER);
    i2s_new_channel(&chan_cfg, &handle, NULL);

    i2s_std_config_t std_cfg = {
    .clk_cfg = I2S_STD_CLK_DEFAULT_CONFIG(16000),
    .slot_cfg = I2S_STD_PHILIPS_SLOT_DEFAULT_CONFIG(I2S_DATA_BIT_WIDTH_32BIT, I2S_SLOT_MODE_MONO),
    .gpio_cfg = {
        .mclk = I2S_GPIO_UNUSED,
        .bclk = GPIO_NUM_21,
        .ws = GPIO_NUM_7,
        .dout = GPIO_NUM_18,
        .din = I2S_GPIO_UNUSED,
        .invert_flags = {
            .mclk_inv = false,
            .bclk_inv = false,
            .ws_inv = false,
        },
    },
    
};
    

    i2s_channel_init_std_mode(handle, &std_cfg);
    return handle;
}

void start_ou(i2s_chan_handle_t* handle)
{
    i2s_channel_enable(*handle);
}

void stop_ou(i2s_chan_handle_t* handle)
{
    i2s_channel_disable(*handle);
}

/*
    Metóda prevzatá  a upravená z: https://github.com/atomic14/esp32_sdcard_audio/blob/main/idf-wav-sdcard/lib/audio_output/src/Output.cpp
*/
void write_ou(i2s_chan_handle_t* handle, int16_t* samples, int count) 
{
    int16_t *frames = (int16_t *)malloc(2 * sizeof(int16_t) *  FRAMES);
    int sample_index = 0;
     while (sample_index < count)
    {
        int samples_to_send = 0;
        for (int i = 0; i <  FRAMES && sample_index < count; i++)
        {
            int sample = (samples[sample_index]); 
            frames[i * 2] = sample;
            frames[i * 2 + 1] = sample;
            samples_to_send++;
            sample_index++;
        }
        
        size_t bytes_written = 0;
        i2s_channel_write(*handle, frames, samples_to_send * sizeof(int16_t) * 2, &bytes_written, portMAX_DELAY);
        if (bytes_written != samples_to_send * sizeof(int16_t) * 2)
        {
            ESP_LOGE(TAG_OUT, "Did not write all bytes");
        }
    }
  free(frames);
}


