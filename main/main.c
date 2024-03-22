

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
#include "esp_pthread.h"
#include "pthread.h"

static const char *TAG = "boot";

#define USR_BTN_1 GPIO_NUM_14
#define USR_BTN_2 GPIO_NUM_47

#define VELKOST_MENU 3
// sdcard
#define PIN_NUM_MISO GPIO_NUM_13
#define PIN_NUM_CLK GPIO_NUM_12
#define PIN_NUM_MOSI GPIO_NUM_11
#define PIN_NUM_CS GPIO_NUM_10

typedef struct menu_data {
    // pthread_mutex_t menu_mutex;
    // pthread_cond_t posun;
    // pthread_cond_t potvrdenie;
    int pozicia;
    bool potvrdenie_tl;
    bool posun_tl;
    SSD1306_t * oled;
    int velkost_menu;
    int index_menu;
    char hlavne_menu[VELKOST_MENU][10];
    int zvolene;
    bool start;

} MENU_DATA;

void wait_for_button_push(void* thr_data)
{
    MENU_DATA *data = (MENU_DATA *) thr_data;

    
        while(gpio_get_level(USR_BTN_1) == 1) 
        {
            if (gpio_get_level(USR_BTN_1) == 0)
            {
                //pthread_cond_signal(data->posun);
                data->posun_tl = true;
                //data->index_menu++;
                ESP_LOGI(TAG, "Stlacil");
            }
            
            vTaskDelay(20);
        }
    }

void concatenate_string(char* s, char* s1)
{
    int i;
 
    int j = strlen(s);
 
    for (i = 0; s1[i] != '\0'; i++) {
        s[i + j] = s1[i];
    }
 
    s[i + j] = '\0';
 
    return;
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


void vypis_nahravok() 
{
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
}

void posun_menu(void* thr_data) 
{

    MENU_DATA *data = (MENU_DATA *) thr_data;
        bool invert = false;
        int pocet_riadkov = 0;
        if (data->index_menu > 1) {
            
            data->pozicia = 2;
            if (data->velkost_menu > data->index_menu) {
                if ((data->velkost_menu - data->index_menu) > 1) {
                        pocet_riadkov = 2;
                } else {
                    pocet_riadkov = data->velkost_menu - data->index_menu;
                    ssd1306_clear_line(data->oled, data->pozicia, false);
                    ssd1306_clear_line(data->oled, data->pozicia + 1, false);
                }
                for (int i = 0; i < pocet_riadkov; i++) 
                {
                    if (data->pozicia >= 4) 
                    {
                        break;
                    }
                    if (data->zvolene + 1 == data->index_menu) {
                        data->zvolene = data->index_menu;
                        invert = true;
                    }
                    ssd1306_clear_line(data->oled, data->pozicia + i, false);
                    ssd1306_display_text(data->oled, data->pozicia + i, data->hlavne_menu[data->index_menu + i], strlen(data->hlavne_menu[data->index_menu + i]), invert);
                    data->index_menu += 1;
                    data->pozicia += 1; 
                    
		            vTaskDelay(500 / portTICK_PERIOD_MS);
                    //TODO: dorobit ak by bolo viac ako 4 moznosti v menu
                }
            } else {
                data->pozicia = 2;
                data->index_menu = 0;
                data->zvolene = 0;
                data->start = true;
                bool jano = false;
                for (int i = 0; i < 2; i++)
                {
                    if(!jano) {
                        invert = true;
                        jano = true;
                        data->zvolene = data->index_menu;
                    } else {
                        invert = false;    
                    }
                
                    ssd1306_clear_line(data->oled, data->pozicia + i, false);
                    ssd1306_display_text(data->oled, data->pozicia + i, data->hlavne_menu[data->index_menu+ i], strlen(data->hlavne_menu[data->index_menu + i]), invert);
                    
                }
            }
        } else {
            ESP_LOGI(TAG, "Start");
            bool jano = false;
            for (int i = 0; i < 2; i++)
            {
                if(jano) {
                    invert = true;
                    data->zvolene = data->index_menu;
                } else {
                    jano = true;
                }
                ssd1306_clear_line(data->oled, data->pozicia + i, false);
                ssd1306_display_text(data->oled, data->pozicia + i, data->hlavne_menu[data->index_menu], strlen(data->hlavne_menu[data->index_menu]), invert);
                
                ++data->index_menu;
            }
            
            
        }
    }




void menu(MENU_DATA* thr_data) 
{
    MENU_DATA *data = (MENU_DATA *) thr_data;
    int center, top, bottom;
    top = 1;
	center = 1;
	bottom = 4;
    
    
    //pthread_mutex_lock(data->menu_mutex);
    char lineChar[20];
    
    
    ssd1306_clear_screen(data->oled, false);
    ssd1306_display_text(data->oled, 0, "ESP32-S3-Audio", 15, false);
    ssd1306_display_text(data->oled, 1, "--------------", 15, false);

    ssd1306_display_text(data->oled, 2, data->hlavne_menu[data->index_menu], strlen(data->hlavne_menu[data->index_menu]), true);
    ssd1306_display_text(data->oled, 3, data->hlavne_menu[data->index_menu + 1], strlen(data->hlavne_menu[data->index_menu + 1]), false);
    data->pozicia = 2;
    //pthread_mutex_unlock(data->menu_mutex);
        
    while (true)
    {
        // while (!data->bolo_stlacene)
        // {
        //     pthread_cond_wait(data->bolo_stlacene, data->menu_mutex);
        // }
       
        wait_for_button_push(data);
       
        posun_menu(data);
         ESP_LOGI(TAG, "Zvolene: %d", data->zvolene);
                //ak bol posun
        //ssd1306_display_text(data->oled, ++data->pozicia, hlavne_menu[++index_menu], 6, false);

        

        // vTaskDelay(1000/portTICK_PERIOD_MS);
        // ssd1306_software_scroll(data->oled, 2, (data->oled->_pages-1) );
        // for (int line=0;line<bottom+10;line++) {
	    // if ( (line % (data->oled->_pages-1)) == 0) ssd1306_scroll_clear(data->oled);
		// ssd1306_scroll_text(data->oled, hlavne_menu[2], strlen(hlavne_menu[2]), false);
		vTaskDelay(500 / portTICK_PERIOD_MS);
	}
        
        
        
}
    
  
    





void app_main(void)
{
    SSD1306_t oled;
    ESP_LOGI(TAG, "INTERFACE is i2c");
	ESP_LOGI(TAG, "CONFIG_SDA_GPIO=%d",CONFIG_SDA_GPIO);
	ESP_LOGI(TAG, "CONFIG_SCL_GPIO=%d",CONFIG_SCL_GPIO);
	ESP_LOGI(TAG, "CONFIG_RESET_GPIO=%d",CONFIG_RESET_GPIO);
	i2c_master_init(&oled, CONFIG_SDA_GPIO, CONFIG_SCL_GPIO, CONFIG_RESET_GPIO);
    ESP_LOGI(TAG, "Panel is 128x32");
	ssd1306_init(&oled, 128, 32);
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
     gpio_pullup_en(USR_BTN_2);
    gpio_set_direction(USR_BTN_2, GPIO_MODE_DEF_INPUT);

    WAVFILEWRITER writer;
    writer.m_header = header;

    WAVFILEREADER reader;
    reader.m_wav_header = header;
    

    MENU_DATA menu_data =  {
        .hlavne_menu = {"Prehrat\0", "Nahrat\0", "Wifi\0"},
        .index_menu = 0,
        .oled = &oled,
        .posun_tl = false,
        .potvrdenie_tl = false,
        .pozicia = 0,
        .velkost_menu = 3,
        .zvolene = 0,
        .start= true
    };
    

        
        
    while (true) 
    {
        // wait_for_button_push();
        // record(&input_handle, &writer, "/sdcard/test.wav");
        // wait_for_button_push();
        // play(&output_handle, &reader,"/sdcard/test.wav" );
        menu(&menu_data);
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
   

    
}
