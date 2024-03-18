

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
#include "esp_timer.h"
#include "ssd1306.h"
#include <sys/dirent.h>
#include <string.h>

static const char *TAG = "boot";

#define USR_BTN_1 GPIO_NUM_14

// sdcard
#define PIN_NUM_MISO GPIO_NUM_13
#define PIN_NUM_CLK GPIO_NUM_12
#define PIN_NUM_MOSI GPIO_NUM_11
#define PIN_NUM_CS GPIO_NUM_10


void wait_for_button_push()
{
    while(gpio_get_level(USR_BTN_1) == 1) 
    {
        vTaskDelay(20);
    }
}

void record(i2s_chan_handle_t * handle, WAVFILEWRITER* writer, const char *fname) 
{
    int16_t *samples = (int16_t*) malloc(sizeof(int16_t) * 1024);
    ESP_LOGI(TAG, "Start recording");
    start_in(handle);
    FILE* fp = fopen(fname, "wb");
    WAVFileWriter_init(writer, fp, 44100);
    while (gpio_get_level(USR_BTN_1) == 0) 
    {
        int samples_read = read_i2s(handle, samples, 1024);
        int64_t start = esp_timer_get_time();
        write_wr(writer, samples, samples_read);
        int64_t end = esp_timer_get_time();
        ESP_LOGI(TAG, "Wrote %d samples in %lld microseconds", samples_read, end - start);
    }
    stop_in(handle);
    finish(writer);
    fclose(fp);
    free(samples);
    ESP_LOGI(TAG, "Finished recording");
    
} 

void play(i2s_chan_handle_t * handle, WAVFILEREADER* reader, const char *fname)
{
    int16_t * samples = (int16_t*)malloc(sizeof(int16_t) * 1024);

    FILE* fp = fopen(fname, "rb");

    WAVFileReader(reader, fp);

    ESP_LOGI(TAG, "Start playing");
    start_ou(handle);
    ESP_LOGI(TAG, "Opened wav file");
    
    while (true)
    {
        int samples_read = read_fr(reader, samples, 1024);
        if (samples_read == 0)
    {
        break;
    }
    ESP_LOGI(TAG, "Read %d samples", samples_read);
    write_ou(handle, samples, samples_read);
    ESP_LOGI(TAG, "Played samples");
    }

    stop_ou(handle);
    fclose(fp);

    free(samples);
    ESP_LOGI(TAG, "Finished playing");
}




void app_main(void)
{
    SSD1306_t oled;
    int center, top, bottom;
    char lineChar[20];

    wav_header_t header;
    init_header(&header);

    SDCARD card;
    ESP_LOGI(TAG, "Starting up");
    ESP_LOGI(TAG, "Mounting SDCard on /sdcard");
    SdCard_init(&card, "/sdcard", PIN_NUM_MISO, PIN_NUM_MOSI, PIN_NUM_CLK, PIN_NUM_CS);

    i2s_chan_handle_t input_handle = i2s_in_init();
    i2s_chan_handle_t output_handle = i2s_out_init();

    gpio_pullup_en(USR_BTN_1);
    gpio_set_direction(USR_BTN_1, GPIO_MODE_DEF_INPUT);

    WAVFILEWRITER writer;
    writer.m_header = header;

    WAVFILEREADER reader;
    reader.m_wav_header = header;

    DIR* dir = opendir("/sdcard");
    if (!dir) {
        ESP_LOGE(TAG, "Failed to open directory.");
        return;
    }

    // Read directory entries
    struct dirent* entry;
    int sum_files = 0;
    int max_length = 0;

    while ((entry = readdir(dir)) != NULL) {
        sum_files++;
        //ESP_LOGI(TAG, "%s", entry->d_name);
        int i = 0;
        while (entry->d_name[i] !='\0') {
            i++;
        }
        if (max_length < i) 
        {
            max_length = i;
        }
    }

    closedir(dir);
    char file_names[sum_files][20];
    dir = opendir("/sdcard");
    int pom = 0;
    while ((entry = readdir(dir)) != NULL) {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue;
        }
        //ESP_LOGI(TAG, "%s", entry->d_name);
        strcpy(file_names[pom], entry->d_name);
        pom++;
        
        
    }
    closedir(dir);
    ESP_LOGI(TAG, "Pocet suborov na sd karte: %d", sum_files);
    for (int i = 0; i < sum_files; i++)
    {
        ESP_LOGI(TAG, "%s", file_names[i]);
    }

        
        
    while (true) 
    {
        wait_for_button_push();
        record(&input_handle, &writer, "/sdcard/test.wav");
        wait_for_button_push();
        play(&output_handle, &reader,"/sdcard/test.wav" );

        vTaskDelay(pdMS_TO_TICKS(1000));
    }

    
}
