#include <stdio.h>
#include <string.h>
#include <sys/unistd.h>
#include <sys/stat.h>
#include "esp_err.h"
#include "esp_log.h"
#include "esp_vfs_fat.h"
#include "dirent.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/sdmmc_host.h"
#include "driver/sdspi_host.h"
#include "sdmmc_cmd.h"
#include "sdcard_init.h"

#define SD_CARD_TAG                 "SD_CARD_UTIL"
#define SD_CARD_INTR_GPIO           34
#define SD_CARD_PWR_CTRL            13

#if defined CONFIG_ESP32_KORVO_V1_1_BOARD
#define PIN_NUM_MISO 21
#define PIN_NUM_MOSI 18
#define PIN_NUM_CLK  5
#define PIN_NUM_CS   23
#endif

esp_err_t sd_card_mount(const char* basePath)
{
#if defined CONFIG_ESP_LYRAT_V4_3_BOARD || defined CONFIG_ESP_LYRAT_MINI_V1_1_BOARD
    sdmmc_host_t host = SDMMC_HOST_DEFAULT();

#ifdef CONFIG_ESP_LYRAT_V4_3_BOARD
    // To use 1-line SD mode, uncomment the following line:
    host.flags = SDMMC_HOST_FLAG_1BIT;
    host.max_freq_khz = SDMMC_FREQ_HIGHSPEED;

    sdmmc_slot_config_t slot_config = SDMMC_SLOT_CONFIG_DEFAULT();
    slot_config.gpio_cd = SD_CARD_INTR_GPIO;

#elif defined CONFIG_ESP_LYRAT_MINI_V1_1_BOARD
    gpio_config_t sdcard_pwr_pin_cfg = {
        .pin_bit_mask = 1UL << SD_CARD_PWR_CTRL,
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    gpio_config(&sdcard_pwr_pin_cfg);
    gpio_set_level(SD_CARD_PWR_CTRL, 0);

    host.flags = SDMMC_HOST_FLAG_1BIT;

    sdmmc_slot_config_t slot_config = SDMMC_SLOT_CONFIG_DEFAULT();
    slot_config.gpio_cd = SD_CARD_INTR_GPIO;
    slot_config.width = 1;

#endif

    esp_vfs_fat_sdmmc_mount_config_t mount_config = {
        .format_if_mount_failed = false,
        .max_files = 5
    };

    sdmmc_card_t *card;
    esp_err_t ret = esp_vfs_fat_sdmmc_mount(basePath, &host, &slot_config, &mount_config, &card);

    switch (ret) {
        case ESP_OK:
            // Card has been initialized, print its properties
            sdmmc_card_print_info(stdout, card);
            ESP_LOGI(SD_CARD_TAG, "CID name %s!\n", card->cid.name);
            break;
        case ESP_ERR_INVALID_STATE:
            ESP_LOGE(SD_CARD_TAG, "File system already mounted");
            break;
        case ESP_FAIL:
            ESP_LOGE(SD_CARD_TAG, "Failed to mount filesystem. If you want the card to be formatted, set format_if_mount_failed = true.");
            break;
        default:
            ESP_LOGE(SD_CARD_TAG, "Failed to initialize the card (%d). Make sure SD card lines have pull-up resistors in place.", ret);
            break;
    }

#elif defined CONFIG_ESP32_KORVO_V1_1_BOARD

    ESP_LOGI("APP_TAG", "Initializing SD card");
    ESP_LOGI("APP_TAG", "Using SPI peripheral");

    sdmmc_host_t host = SDSPI_HOST_DEFAULT();
    sdspi_slot_config_t slot_config = SDSPI_SLOT_CONFIG_DEFAULT();
    slot_config.gpio_miso = PIN_NUM_MISO;
    slot_config.gpio_mosi = PIN_NUM_MOSI;
    slot_config.gpio_sck  = PIN_NUM_CLK;
    slot_config.gpio_cs   = PIN_NUM_CS;

    esp_vfs_fat_sdmmc_mount_config_t mount_config = {
        .format_if_mount_failed = false,
        .max_files = 5,
        .allocation_unit_size = 16 * 1024
    };

    sdmmc_card_t* card;
    esp_err_t ret = esp_vfs_fat_sdmmc_mount("/sdcard", &host, &slot_config, &mount_config, &card);

    if (ret != ESP_OK) {
        if (ret == ESP_FAIL) {
            ESP_LOGE("APP_TAG", "Failed to mount filesystem. "
                     "If you want the card to be formatted, set format_if_mount_failed = true.");
        } else {
            ESP_LOGE("APP_TAG", "Failed to initialize the card (%s). "
                     "Make sure SD card lines have pull-up resistors in place.", esp_err_to_name(ret));
        }
        return;
    }
    sdmmc_card_print_info(stdout, card);

#endif

    return ret;

}


int FatfsComboWrite(const void* buffer, int size, int count, FILE* stream)
{
    int res = 0;
    res = fwrite(buffer, size, count, stream);
    res |= fflush(stream);        // required by stdio, this will empty any buffers which newlib holds
    res |= fsync(fileno(stream)); // this will tell the filesystem driver to write data to disk

    return res;
}

