/**
 *
 * @copyright Copyright 2021 Espressif Systems (Shanghai) Co. Ltd.
 *
 *      Licensed under the Apache License, Version 2.0 (the "License");
 *      you may not use this file except in compliance with the License.
 *      You may obtain a copy of the License at
 *
 *               http://www.apache.org/licenses/LICENSE-2.0
 *
 *      Unless required by applicable law or agreed to in writing, software
 *      distributed under the License is distributed on an "AS IS" BASIS,
 *      WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *      See the License for the specific language governing permissions and
 *      limitations under the License.
 */

#include "string.h"
#include "bsp_board.h"
#if ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(5, 0, 0)
#include "driver/i2s_std.h"
#include "driver/i2s_tdm.h"
#include "soc/soc_caps.h"
#else
#include "driver/i2s.h"
#endif
#include "driver/gpio.h"
#include "esp_err.h"
#include "esp_log.h"
#include "esp_rom_sys.h"
#include "esp_vfs_fat.h"
#include "sdmmc_cmd.h"
#if ((SOC_SDMMC_HOST_SUPPORTED) && (FUNC_SDMMC_EN))
#include "driver/sdmmc_host.h"
#endif /* ((SOC_SDMMC_HOST_SUPPORTED) && (FUNC_SDMMC_EN)) */

#define GPIO_MUTE_NUM   GPIO_NUM_1
#define GPIO_MUTE_LEVEL 1
#define ACK_CHECK_EN   0x1     /*!< I2C master will check ack from slave*/
#define ADC_I2S_CHANNEL 2
static sdmmc_card_t *card;
static const char *TAG = "board";
static int s_play_sample_rate = 16000;
static int s_play_channel_format = 1;
static int s_bits_per_chan = 16;

#if ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(5, 0, 0)
static i2s_chan_handle_t                rx_handle = NULL;        // I2S rx channel handler
#endif

static esp_err_t bsp_i2s_init(i2s_port_t i2s_num, uint32_t sample_rate, int channel_format, int bits_per_chan)
{
    esp_err_t ret_val = ESP_OK;

#if ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(5, 0, 0)
    i2s_chan_config_t chan_cfg = I2S_CHANNEL_DEFAULT_CONFIG(i2s_num, I2S_ROLE_MASTER);

    ret_val |= i2s_new_channel(&chan_cfg, NULL, &rx_handle);
    i2s_std_config_t std_cfg = I2S_CONFIG_DEFAULT(16000, I2S_SLOT_MODE_MONO, 32);
    std_cfg.slot_cfg.slot_mask = I2S_STD_SLOT_LEFT;
    // std_cfg.clk_cfg.mclk_multiple = EXAMPLE_MCLK_MULTIPLE;   //The default is I2S_MCLK_MULTIPLE_256. If not using 24-bit data width, 256 should be enough
    ret_val |= i2s_channel_init_std_mode(rx_handle, &std_cfg);
    ret_val |= i2s_channel_enable(rx_handle);
#else
    // i2s_config_t i2s_config = I2S_CONFIG_DEFAULT(16000, I2S_CHANNEL_FMT_ONLY_LEFT, 32);
    i2s_config_t i2s_config = I2S_CONFIG_DEFAULT(sample_rate, I2S_CHANNEL_FMT_ONLY_LEFT, bits_per_chan);

    i2s_pin_config_t pin_config = {
        .bck_io_num = GPIO_I2S_SCLK,
        .ws_io_num = GPIO_I2S_LRCK,
        .data_out_num = GPIO_I2S_DOUT,
        .data_in_num = GPIO_I2S_SDIN,
        .mck_io_num = GPIO_I2S_MCLK,
    };

    ret_val |= i2s_driver_install(i2s_num, &i2s_config, 0, NULL);
    ret_val |= i2s_set_pin(i2s_num, &pin_config);
#endif

    return ret_val;
}

static esp_err_t bsp_i2s_deinit(i2s_port_t i2s_num)
{
    esp_err_t ret_val = ESP_OK;

#if ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(5, 0, 0)
    ret_val |= i2s_channel_disable(rx_handle);
    ret_val |= i2s_del_channel(rx_handle);
    rx_handle = NULL;
#else
    ret_val |= i2s_stop(i2s_num);
    ret_val |= i2s_driver_uninstall(i2s_num);
#endif

    return ret_val;
}

esp_err_t bsp_get_feed_data(bool is_get_raw_channel, int16_t *buffer, int buffer_len)
{
    esp_err_t ret = ESP_OK;
    size_t bytes_read;
    int audio_chunksize = buffer_len / (sizeof(int32_t));
#if ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(5, 0, 0)
    ret = i2s_channel_read(rx_handle, buffer, buffer_len, &bytes_read, portMAX_DELAY);
#else
    ret = i2s_read(I2S_NUM_1, buffer, buffer_len, &bytes_read, portMAX_DELAY);
#endif

    int32_t *tmp_buff = buffer;
    for (int i = 0; i < audio_chunksize; i++) {
        tmp_buff[i] = tmp_buff[i] >> 14; // 32:8为有效位， 8:0为低8位， 全为0， AFE的输入为16位语音数据，拿29：13位是为了对语音信号放大。
    }

    return ret;
}

int bsp_get_feed_channel(void)
{
    return ADC_I2S_CHANNEL;
}

char* bsp_get_input_format(void)
{
    return "NM";
}


esp_err_t bsp_board_init(uint32_t sample_rate, int channel_format, int bits_per_chan)
{
    bsp_i2s_init(I2S_NUM_1, 16000, 2, 32);

    return ESP_OK;
}

esp_err_t bsp_sdcard_init(char *mount_point, size_t max_files)
{
    if (NULL != card) {
        return ESP_ERR_INVALID_STATE;
    }

    /* Check if SD crad is supported */
    if (!FUNC_SDMMC_EN && !FUNC_SDSPI_EN) {
        ESP_LOGE(TAG, "SDMMC and SDSPI not supported on this board!");
        return ESP_ERR_NOT_SUPPORTED;
    }

    esp_err_t ret_val = ESP_OK;

    /**
     * @brief Options for mounting the filesystem.
     *   If format_if_mount_failed is set to true, SD card will be partitioned and
     *   formatted in case when mounting fails.
     *
     */
    esp_vfs_fat_sdmmc_mount_config_t mount_config = {
        .format_if_mount_failed = false,
        .max_files = max_files,
        .allocation_unit_size = 16 * 1024
    };

    /**
     * @brief Use settings defined above to initialize SD card and mount FAT filesystem.
     *   Note: esp_vfs_fat_sdmmc/sdspi_mount is all-in-one convenience functions.
     *   Please check its source code and implement error recovery when developing
     *   production applications.
     *
     */
    sdmmc_host_t host =
#if FUNC_SDMMC_EN
        SDMMC_HOST_DEFAULT();
#else
        SDSPI_HOST_DEFAULT();
    spi_bus_config_t bus_cfg = {
        .mosi_io_num = GPIO_SDSPI_MOSI,
        .miso_io_num = GPIO_SDSPI_MISO,
        .sclk_io_num = GPIO_SDSPI_SCLK,
        .quadwp_io_num = GPIO_NUM_NC,
        .quadhd_io_num = GPIO_NUM_NC,
        .max_transfer_sz = 4000,
    };
    ret_val = spi_bus_initialize(host.slot, &bus_cfg, SPI_DMA_CH_AUTO);
    if (ret_val != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize bus.");
        return ret_val;
    }
#endif

    /**
     * @brief This initializes the slot without card detect (CD) and write protect (WP) signals.
     *   Modify slot_config.gpio_cd and slot_config.gpio_wp if your board has these signals.
     *
     */
#if FUNC_SDMMC_EN
    sdmmc_slot_config_t slot_config = SDMMC_SLOT_CONFIG_DEFAULT();
#else
    sdspi_device_config_t slot_config = SDSPI_DEVICE_CONFIG_DEFAULT();
#endif

#if FUNC_SDMMC_EN
    /* Config SD data width. 0, 4 or 8. Currently for SD card, 8 bit is not supported. */
    slot_config.width = SDMMC_BUS_WIDTH;

    /**
     * @brief On chips where the GPIOs used for SD card can be configured, set them in
     *   the slot_config structure.
     *
     */
#if SOC_SDMMC_USE_GPIO_MATRIX
    slot_config.clk = GPIO_SDMMC_CLK;
    slot_config.cmd = GPIO_SDMMC_CMD;
    slot_config.d0 = GPIO_SDMMC_D0;
    slot_config.d1 = GPIO_SDMMC_D1;
    slot_config.d2 = GPIO_SDMMC_D2;
    slot_config.d3 = GPIO_SDMMC_D3;
#endif
    slot_config.cd = GPIO_SDMMC_DET;
    slot_config.flags |= SDMMC_SLOT_FLAG_INTERNAL_PULLUP;
#else
    slot_config.gpio_cs = GPIO_SDSPI_CS;
    slot_config.host_id = host.slot;
#endif
    /**
     * @brief Enable internal pullups on enabled pins. The internal pullups
     *   are insufficient however, please make sure 10k external pullups are
     *   connected on the bus. This is for debug / example purpose only.
     */

    /* get FAT filesystem on SD card registered in VFS. */
    ret_val =
#if FUNC_SDMMC_EN
        esp_vfs_fat_sdmmc_mount(mount_point, &host, &slot_config, &mount_config, &card);
#else
        esp_vfs_fat_sdspi_mount(mount_point, &host, &slot_config, &mount_config, &card);
#endif

    /* Check for SDMMC mount result. */
    if (ret_val != ESP_OK) {
        if (ret_val == ESP_FAIL) {
            ESP_LOGE(TAG, "Failed to mount filesystem. "
                     "If you want the card to be formatted, set the EXAMPLE_FORMAT_IF_MOUNT_FAILED menuconfig option.");
        } else {
            ESP_LOGE(TAG, "Failed to initialize the card (%s). "
                     "Make sure SD card lines have pull-up resistors in place.", esp_err_to_name(ret_val));
        }
        return ret_val;
    }

    /* Card has been initialized, print its properties. */
    sdmmc_card_print_info(stdout, card);

    return ret_val;
}

esp_err_t bsp_sdcard_deinit(char *mount_point)
{
    if (NULL == mount_point) {
        return ESP_ERR_INVALID_STATE;
    }

    /* Unmount an SD card from the FAT filesystem and release resources acquired */
    esp_err_t ret_val = esp_vfs_fat_sdcard_unmount(mount_point, card);

    /* Make SD/MMC card information structure pointer NULL */
    card = NULL;

    return ret_val;
}