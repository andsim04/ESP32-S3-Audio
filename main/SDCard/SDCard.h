

#include <hal/gpio_types.h>
#include <driver/sdmmc_types.h>
#include <driver/sdspi_host.h>


#ifndef _SDCARD_H
#define _SDCARD_H

typedef struct SDCard {
    char* m_mount_point;
    gpio_num_t miso;
    gpio_num_t mosi;
    gpio_num_t clk;
    gpio_num_t cs;
    sdmmc_card_t *m_card;
    sdmmc_host_t m_host;

} SDCARD;

void SdCard_init(SDCARD * card, const char *mount_point, gpio_num_t miso, gpio_num_t mosi, gpio_num_t clk, gpio_num_t cs);

void freeCard(SDCARD * card);

#endif 