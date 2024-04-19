

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
    int pozicia;
    SSD1306_t * oled;
    int velkost_menu;
    int index_menu;
    char menu_vyber[VELKOST_MENU][10];
    char ** nahravky;
    int zvolene;
    bool start;
    i2s_chan_handle_t * handle_in;
    i2s_chan_handle_t * handle_ou;
    WAVFILEWRITER * writer;
    WAVFILEREADER * reader;
    char* kategoria;
    
} MENU_DATA;

bool wait_for_button_push()
{
   
        while(gpio_get_level(USR_BTN_2) == 1 && gpio_get_level(USR_BTN_1) == 1) 
        {
            
            vTaskDelay(20);
        }

        if (gpio_get_level(USR_BTN_1) == 0)
            {
                ESP_LOGI(TAG, "Stlacil 3");
                return true;
            } else {
                ESP_LOGI(TAG, "Stlacil 4");
                return false;
            }
        return false;
             return false;
         
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

void record_play(i2s_chan_handle_t * handle_in, i2s_chan_handle_t * handle_ou)
{
    int16_t *samples = (int16_t*) malloc(sizeof(int16_t) * 1024);
    ESP_LOGI(TAG, "Start recording");
    start_in(handle_in);
    start_ou(handle_ou);
    size_t bytes_read = 0;
    while (gpio_get_level(USR_BTN_1) == 0)
    {

       
       
       int samples_read = read_i2s(handle_in, samples, 1024);
 
       write_ou(handle_ou, samples, samples_read);
   
       vTaskDelay(pdMS_TO_TICKS(1));
    }
    

    stop_in(handle_in);
    stop_ou(handle_ou);

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


void vypis_nahravok(MENU_DATA * data) 
{
    DIR* dir = opendir("/sdcard/nahravky");
    if (!dir) {
        ESP_LOGE(TAG, "Failed to open directory.");
        return;
    }

    // Read directory entries
    struct dirent* entry;
    int sum_files = 0;

    while ((entry = readdir(dir)) != NULL) {
        sum_files++;
        //ESP_LOGI(TAG, "%s", entry->d_name);
        int i = 0;
        while (entry->d_name[i] !='\0') {
            i++;
        }
        
    }

    closedir(dir);
    char file_names[sum_files][16];
    dir = opendir("/sdcard/nahravky");
    int pom = 0;
    while ((entry = readdir(dir)) != NULL) {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue;
        }
        strcpy(file_names[pom], entry->d_name);
        pom++;
        
        
    }
    closedir(dir);

    data->nahravky = (char**)malloc((sum_files + 1) * sizeof(char*));
    for (int i = 0; i < sum_files + 1; i++)
    {
        data->nahravky[i] = (char*)malloc(16 * sizeof(char));
    }
    
    strcpy(data->nahravky[0], "naspat\0");
    for (int i = 1; i < sum_files + 1; i++)
    {
       strcpy(data->nahravky[i], file_names[i - 1]);   
    }
    
    data->velkost_menu = sum_files + 1;

   
    

    ESP_LOGI(TAG, "Pocet suborov na sd karte: %d", sum_files);
    for (int i = 0; i < sum_files + 1; i++)
    {
        ESP_LOGI(TAG, "%s", data->nahravky[i]);
        ESP_LOGI(TAG, "%s", file_names[i]);
    }
}

void posun_menu(MENU_DATA * data) 
{
    
    bool invert;
    int pocet_riadkov;
    if ((data->velkost_menu - data->index_menu) > 1) {
        pocet_riadkov = 2;
    }
    else if ((data->velkost_menu - data->index_menu) <= 0) {
        data->index_menu = 0;
        data->start = true;
        pocet_riadkov = 2;
    } else {
        pocet_riadkov = 1;
    }
        
        

    if (data->start) 
    {
            
        data->start = false;
        data->zvolene = 0;
            

    } else {
        data->zvolene++;
    }

        

    ssd1306_clear_line(data->oled, data->pozicia, false);
    ssd1306_clear_line(data->oled, data->pozicia + 1, false);

    for (int i = 0; i < pocet_riadkov; i++)
    {
        if ((i % 2) == 0) {
            invert = true;
        } else {
            invert = false;
        }
        if (strcmp(data->kategoria, "hlavne") == 0) { //zatial dve ak bude treba da sa rozsirit
            ssd1306_display_text(data->oled, data->pozicia + i, data->menu_vyber[data->index_menu + i], strlen(data->menu_vyber[data->index_menu + i]), invert);
        } else {
            ssd1306_display_text(data->oled, data->pozicia + i, data->nahravky[data->index_menu + i],strlen(data->nahravky[data->index_menu + i]) , invert);
        }
    }
    
        
    data->index_menu++;
}

void potvrdenie_menu(MENU_DATA* data) {

    if (strcmp(data->kategoria, "hlavne") == 0) {

    
        switch (data->zvolene)
        {
        case 0:
            data->kategoria = "prehrat";
            data->zvolene = 0;
            data->index_menu = 0;
            vypis_nahravok(data);
            data->start = true;
            posun_menu(data);
            break;
        case 1:
            record(data->handle_in, data->writer, "test"); // dorobit play
            break;
        case 2:

            break;
        default:
            break;
        }
    } else {
        if (data->zvolene == 0) {
            data->kategoria = "hlavne";
            data->index_menu = 0;
            free(data->nahravky);
            data->velkost_menu = VELKOST_MENU;
            posun_menu(data); 
            
        } else {
            ;
            char* cesta = "/sdcard/nahravky/";
            concatenate_string(cesta, data->nahravky[data->zvolene]);
            play(data->handle_ou, data->reader, cesta);
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
    
    
   
    char lineChar[20];
    
    
    ssd1306_clear_screen(data->oled, false);
    ssd1306_display_text(data->oled, 0, "ESP32-S3-Audio", 15, false);
    ssd1306_display_text(data->oled, 1, "--------------", 15, false);

 
    ssd1306_display_text(data->oled, 2, "Stlacte tla 3/4", 16, true);
    data->pozicia = 2;
    data->zvolene = 0;
   
        
    while (true)
    {
        if (wait_for_button_push()) {
            posun_menu(data);
        } else {
            potvrdenie_menu(data);
        }
        
		vTaskDelay(200/ portTICK_PERIOD_MS);
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
    
    char * kategoria = "hlavne";
    MENU_DATA menu_data =  {
        .menu_vyber = {"Prehrat\0", "Nahrat\0", "Wifi\0"},
        .index_menu = 0,
        .oled = &oled,
        .pozicia = 0,
        .velkost_menu = 3,
        .zvolene = 0,
        .start= true,
        .handle_in = &input_handle,
        .handle_ou = &output_handle,
        .reader = &reader,
        .writer = &writer,
        .kategoria = kategoria
    };
    

        
        
    while (true) 
    {
        //wait_for_button_push();
        //record_play(&input_handle, &output_handle);
        //record(&input_handle, &writer, "/sdcard/test.wav");

        // wait_for_button_push();
        // play(&output_handle, &reader,"/sdcard/test.wav" );
        menu(&menu_data);
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
   

    
}
