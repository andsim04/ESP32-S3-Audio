#ifndef _OUTPUT_H
#define _OUTPUT_H

#include "driver/i2s_std.h"
#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

const int NUM_FRAMES_TO_SEND = 256;
i2s_chan_handle_t* i2s_out_init();
void start_ou(i2s_chan_handle_t* handle);
void stop_ou(i2s_chan_handle_t* handle);
void write_ou(i2s_chan_handle_t* handle, int16_t* samples, int count);


#endif