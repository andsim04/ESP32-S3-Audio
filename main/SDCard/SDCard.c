#include <stdio.h>
#include <string.h>
#include <sys/unistd.h>
#include <sys/stat.h>
#include "esp_err.h"
#include "esp_log.h"
#include "esp_vfs_fat.h"
#include "driver/spi_common.h"
#include "driver/sdspi_host.h"
#include "sdmmc_cmd.h"

#include "SDCard.h"

const char *TAG = "SDC";

#define SPI_DMA_CHAN 1

esp_err_t SdCard_init(SDCARD * card ,const char *mount_point, gpio_num_t miso, gpio_num_t mosi, gpio_num_t clk, gpio_num_t cs)
{
    card->m_mount_point = mount_point;
    sdmmc_host_t host = SDSPI_HOST_DEFAULT();
    host.command_timeout_ms = 1000;
    card->m_host = host;
    esp_err_t ret;

    esp_vfs_fat_sdmmc_mount_config_t mount_config = {
      .format_if_mount_failed = false,
      .max_files = 10,
      .allocation_unit_size = 16 * 1024};

  ESP_LOGI(TAG, "Initializing SD card");

  spi_bus_config_t bus_cfg = {
      .mosi_io_num = mosi,
      .miso_io_num = miso,
      .sclk_io_num = clk,
      .quadwp_io_num = -1,
      .quadhd_io_num = -1,
      .max_transfer_sz = 4092,
      .flags = 0,
      .intr_flags = 0};
  ret = spi_bus_initialize((spi_host_device_t)(card->m_host.slot), &bus_cfg, SPI_DMA_CH_AUTO);
  if (ret != ESP_OK)
  {
    ESP_LOGE(TAG, "Failed to initialize bus.");
    return ret;
  }

  sdspi_device_config_t slot_config = SDSPI_DEVICE_CONFIG_DEFAULT();
  slot_config.gpio_cs = cs;
  slot_config.host_id = (spi_host_device_t)(card->m_host.slot);

  ret = esp_vfs_fat_sdspi_mount(card->m_mount_point, &card->m_host, &slot_config, &mount_config, &card->m_card);
  
  if (ret != ESP_OK)
  {
    if (ret == ESP_FAIL)
    {
      ESP_LOGE(TAG, "Failed to mount filesystem. "
                    "If you want the card to be formatted, set the EXAMPLE_FORMAT_IF_MOUNT_FAILED menuconfig option.");
    }
    else
    {
      ESP_LOGE(TAG, "Failed to initialize the card (%s). "
                    "Make sure SD card lines have pull-up resistors in place.",
               esp_err_to_name(ret));
               
    }
    return ret;
  }
  ESP_LOGI(TAG, "SDCard mounted at: %s", card->m_mount_point);
  sdmmc_card_print_info(stdout, card->m_card);
  return ret;
}

void freeCard(SDCARD* card) 
{
  esp_vfs_fat_sdcard_unmount(card->m_mount_point, card->m_card);
  ESP_LOGI(TAG, "Card unmounted");
  spi_bus_free((spi_host_device_t)(card->m_host.slot));
}
