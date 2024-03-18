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

void SdCard_init(SDCARD * card ,const char *mount_point, gpio_num_t miso, gpio_num_t mosi, gpio_num_t clk, gpio_num_t cs)
{
    card->m_mount_point = mount_point;
    card->m_host.command_timeout_ms = 0;
    card->m_host.flags = SDMMC_HOST_FLAG_SPI | SDMMC_HOST_FLAG_DEINIT_ARG;
    card->m_host.slot = SDSPI_DEFAULT_HOST;
    card->m_host.max_freq_khz = SDMMC_FREQ_DEFAULT;
    card->m_host.io_voltage = 3.3f;
    card->m_host.init = &sdspi_host_init;
    card->m_host.set_bus_width = NULL;
    card->m_host.get_bus_width = NULL;
    card->m_host.set_bus_ddr_mode = NULL;
    card->m_host.set_card_clk = &sdspi_host_set_card_clk;
    card->m_host.set_cclk_always_on = NULL;
    card->m_host.do_transaction = &sdspi_host_do_transaction;
    card->m_host.deinit_p = &sdspi_host_remove_device;
    card->m_host.io_int_enable = &sdspi_host_io_int_enable;
    card->m_host.io_int_wait = &sdspi_host_io_int_wait;
    card->m_host.command_timeout_ms = 0;
    card->m_host.get_real_freq = &sdspi_host_get_real_freq;
    card->m_host.input_delay_phase = SDMMC_DELAY_PHASE_0;
    card->m_host.set_input_delay = NULL;
    esp_err_t ret;

    esp_vfs_fat_sdmmc_mount_config_t mount_config = {
      .format_if_mount_failed = false,
      .max_files = 5,
      .allocation_unit_size = 16 * 1024};

  ESP_LOGI(TAG, "Initializing SD card");

  spi_bus_config_t bus_cfg = {
      .mosi_io_num = mosi,
      .miso_io_num = miso,
      .sclk_io_num = clk,
      .quadwp_io_num = -1,
      .quadhd_io_num = -1,
      .max_transfer_sz = 4000,
      .flags = 0,
      .intr_flags = 0};
  ret = spi_bus_initialize((spi_host_device_t)(card->m_host.slot), &bus_cfg, SPI_DMA_CH_AUTO);
  if (ret != ESP_OK)
  {
    ESP_LOGE(TAG, "Failed to initialize bus.");
    return;
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
    return;
  }
  ESP_LOGI(TAG, "SDCard mounted at: %s", card->m_mount_point);

  // Card has been initialized, print its properties
  sdmmc_card_print_info(stdout, card->m_card);

}


void freeCard(SDCARD* card) 
{
  esp_vfs_fat_sdcard_unmount(card->m_mount_point, card->m_card);
  ESP_LOGI(TAG, "Card unmounted");
  //deinitialize the bus after all devices are removed
  spi_bus_free((spi_host_device_t)(card->m_host.slot));
}
