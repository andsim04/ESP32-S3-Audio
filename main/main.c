

#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_lcd_panel_io.h"
#include "esp_lcd_panel_ops.h"
#include "esp_err.h"
#include "esp_log.h"
#include "driver/i2c_master.h"
#include "SDCard/SDCard.h"
#include "WAV/WAVFile.h"
#include "WAV/WAVFileReader.h"
#include "WAV/WAVFileWriter.h"
#include "Input/Input.h"
#include "Output/Output.h"

static const char *TAG = "boot";

#define USR_BTN_1 GPIO_NUM_14

// sdcard
#define PIN_NUM_MISO GPIO_NUM_13
#define PIN_NUM_CLK GPIO_NUM_12
#define PIN_NUM_MOSI GPIO_NUM_11
#define PIN_NUM_CS GPIO_NUM_10


void wait_for_button_pust()
{
    while(gpio_get_level(USR_BTN_1) == 1) 
    {
        vTaskDelay(20);
    }
}

void record() 
{

} 


void app_main(void)
{
    wav_header_t header;
    init_header(&header);

    SDCARD card;
    ESP_LOGI(TAG, "Starting up");
    ESP_LOGI(TAG, "Mounting SDCard on /sdcard");
    SdCard_init(&card, "/sdcard", PIN_NUM_MISO, PIN_NUM_MOSI, PIN_NUM_CLK, PIN_NUM_CS);
}
