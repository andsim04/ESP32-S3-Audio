#ifndef _INPUT_H
#define _INPUT_H


#include "driver/i2s_std.h"
#include "driver/gpio.h"
#include "soc/i2s_reg.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_err.h"
#include "esp_log.h"




i2s_chan_handle_t i2s_in_init();
void start_in(i2s_chan_handle_t * handle);
int read_i2s(i2s_chan_handle_t * handle, int16_t* samples, int count);
void stop_in(i2s_chan_handle_t * handle);
void delete_channel(i2s_chan_handle_t * handle);

#endif