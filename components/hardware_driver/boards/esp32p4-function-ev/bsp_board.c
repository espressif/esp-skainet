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
#include "driver/i2c.h"
#include "esp_rom_sys.h"
#include "esp_check.h"
#include "esp_vfs_fat.h"
#include "sdmmc_cmd.h"
#if ((SOC_SDMMC_HOST_SUPPORTED) && (FUNC_SDMMC_EN))
#include "driver/sdmmc_host.h"
#endif /* ((SOC_SDMMC_HOST_SUPPORTED) && (FUNC_SDMMC_EN)) */
#include "sd_pwr_ctrl_by_on_chip_ldo.h"
#include "sd_pwr_ctrl_interface.h"
#include "esp_ldo_regulator.h"


#define GPIO_MUTE_NUM   GPIO_NUM_1
#define GPIO_MUTE_LEVEL 1
#define ACK_CHECK_EN   0x1     /*!< I2C master will check ack from slave*/
#define ADC_I2S_CHANNEL 2
static sdmmc_card_t *card;
static const char *TAG = "board";
static int s_play_sample_rate = 16000;
static int s_play_channel_format = 2;
static int s_bits_per_chan = 16;

static sd_pwr_ctrl_handle_t             pwr_ctrl_handle = NULL;
static esp_ldo_channel_handle_t         ldo_audio_board = NULL;
static bool esp_ldo_enabled = false;

#if ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(5, 3, 0)
static i2s_chan_handle_t                tx_handle = NULL;        // I2S tx channel handler
static i2s_chan_handle_t                rx_handle = NULL;        // I2S rx channel handler
#endif

static audio_codec_data_if_t *codec_data_if = NULL;
static audio_codec_ctrl_if_t *codec_ctrl_if = NULL;
static audio_codec_gpio_if_t *codec_gpio_if = NULL;
static audio_codec_if_t *codec_if = NULL;
static esp_codec_dev_handle_t codec_dev = NULL;


esp_err_t bsp_i2c_init(i2c_port_t i2c_num, uint32_t clk_speed)
{
    i2c_config_t i2c_cfg = {
        .mode = I2C_MODE_MASTER,
        .scl_io_num = GPIO_I2C_SCL,
        .sda_io_num = GPIO_I2C_SDA,
        .scl_pullup_en = GPIO_PULLUP_ENABLE,
        .sda_pullup_en = GPIO_PULLUP_ENABLE,
        .master.clk_speed = clk_speed,
    };
    esp_err_t ret = i2c_param_config(i2c_num, &i2c_cfg);
    if (ret != ESP_OK) {
        return ESP_FAIL;
    }
    return i2c_driver_install(i2c_num, i2c_cfg.mode, 0, 0, 0);
}

esp_err_t bsp_audio_set_play_vol(int volume)
{
    if (!codec_dev) {
        ESP_LOGE(TAG, "DAC codec init fail");
        return ESP_FAIL;
    }
    esp_codec_dev_set_out_vol(codec_dev, volume);
    return ESP_OK;
}

esp_err_t bsp_audio_get_play_vol(int *volume)
{
    if (!codec_dev) {
        ESP_LOGE(TAG, "DAC codec init fail");
        return ESP_FAIL;
    }
    esp_codec_dev_get_out_vol(codec_dev, volume);
    return ESP_OK;
}

// static esp_err_t bsp_i2s_init(i2s_port_t i2s_num, uint32_t sample_rate, i2s_channel_fmt_t channel_format, i2s_bits_per_chan_t bits_per_chan)
static esp_err_t bsp_i2s_init(i2s_port_t i2s_num, uint32_t sample_rate, int channel_format, int bits_per_chan)
{
    esp_err_t ret_val = ESP_OK;

#if ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(5, 3, 0)
    i2s_slot_mode_t channel_fmt = I2S_SLOT_MODE_STEREO;
    if (channel_format != 2) {
        ESP_LOGW(TAG, "Unable to configure channel_format %d, reset to 2", channel_format);
        channel_format = 2;
        channel_fmt = I2S_SLOT_MODE_STEREO;
    }

    if (bits_per_chan != 16) {
        ESP_LOGW(TAG, "Unable to configure bits_per_chan %d, reset to 16 ", bits_per_chan);
        bits_per_chan = 16;
    }

    i2s_chan_config_t chan_cfg = I2S_CHANNEL_DEFAULT_CONFIG(i2s_num, I2S_ROLE_MASTER);
    chan_cfg.auto_clear = true;
    ret_val |= i2s_new_channel(&chan_cfg, &tx_handle, &rx_handle);
    i2s_std_config_t std_cfg = I2S_CONFIG_DEFAULT(sample_rate, channel_fmt, bits_per_chan);
    ret_val |= i2s_channel_init_std_mode(tx_handle, &std_cfg);
    ret_val |= i2s_channel_init_std_mode(rx_handle, &std_cfg);
    ret_val |= i2s_channel_enable(tx_handle);
    ret_val |= i2s_channel_enable(rx_handle);
#else
    ESP_LOGE(TAG, "P4 don't support IDF version < V5.3");
#endif

    return ret_val;
}

static esp_err_t bsp_i2s_deinit(i2s_port_t i2s_num)
{
    esp_err_t ret_val = ESP_OK;

#if ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(5, 3, 0)
    if (rx_handle) {
        ret_val |= i2s_channel_disable(rx_handle);
        ret_val |= i2s_del_channel(rx_handle);
        rx_handle = NULL;
    }
    if (tx_handle) {
        ret_val |= i2s_channel_disable(tx_handle);
        ret_val |= i2s_del_channel(tx_handle);
        tx_handle = NULL;
    }
#else
    ESP_LOGE(TAG, "P4 don't support IDF version < V5.3");
#endif

    return ret_val;
}

static esp_err_t bsp_codec_init(int adc_sample_rate, int dac_sample_rate, int dac_channel_format, int dac_bits_per_chan)
{
    esp_err_t ret_val = ESP_OK;

    // ret_val |= bsp_codec_adc_init(adc_sample_rate);
    // ret_val |= bsp_codec_dac_init(dac_sample_rate, dac_channel_format, dac_bits_per_chan);

    // Do initialize of related interface: data_if, ctrl_if and gpio_if
    audio_codec_i2s_cfg_t i2s_cfg = {
        .port = I2S_NUM_1,
#if ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(5, 0, 0)
        .rx_handle = rx_handle,
        .tx_handle = tx_handle,
#endif
    };
    codec_data_if = audio_codec_new_i2s_data(&i2s_cfg);

    audio_codec_i2c_cfg_t i2c_cfg = {.addr = ES8311_CODEC_DEFAULT_ADDR};
    codec_ctrl_if = audio_codec_new_i2c_ctrl(&i2c_cfg);
    codec_gpio_if = audio_codec_new_gpio();
    // New output codec interface
    es8311_codec_cfg_t es8311_cfg = {
        .codec_mode = ESP_CODEC_DEV_WORK_MODE_BOTH,
        .ctrl_if = codec_ctrl_if,
        .gpio_if = codec_gpio_if,
        .pa_pin = GPIO_PWR_CTRL,
        .use_mclk = true,
    };
    codec_if = es8311_codec_new(&es8311_cfg);
    // New output codec device
    esp_codec_dev_cfg_t dev_cfg = {
        .codec_if = codec_if,
        .data_if = codec_data_if,
        .dev_type = ESP_CODEC_DEV_TYPE_IN_OUT,
    };
    codec_dev = esp_codec_dev_new(&dev_cfg);

    esp_codec_dev_sample_info_t fs = {
        .bits_per_sample = dac_bits_per_chan,
        .sample_rate = dac_sample_rate,
        .channel = dac_channel_format,
    };
    esp_codec_dev_set_in_gain(codec_dev, RECORD_VOLUME);
    esp_codec_dev_set_out_vol(codec_dev, PLAYER_VOLUME);
    esp_codec_dev_open(codec_dev, &fs);

    return ret_val;
}

static esp_err_t bsp_codec_deinit()
{
    esp_err_t ret_val = ESP_OK;

    if (codec_dev) {
        esp_codec_dev_close(codec_dev);
        esp_codec_dev_delete(codec_dev);
        codec_dev = NULL;
    }

    // Delete codec interface
    if (codec_if) {
        audio_codec_delete_codec_if(codec_if);
        codec_if = NULL;
    }
    
    // Delete codec control interface
    if (codec_ctrl_if) {
        audio_codec_delete_ctrl_if(codec_ctrl_if);
        codec_ctrl_if = NULL;
    }
    
    if (codec_gpio_if) {
        audio_codec_delete_gpio_if(codec_gpio_if);
        codec_gpio_if = NULL;
    }
    
    // Delete codec data interface
    if (codec_data_if) {
        audio_codec_delete_data_if(codec_data_if);
        codec_data_if = NULL;
    }

    return ret_val;
}

esp_err_t bsp_audio_play(const int16_t* data, int length, TickType_t ticks_to_wait)
{
    esp_err_t ret = ESP_OK;
    if (!codec_dev) {
        return ESP_FAIL;
    }
    ret = esp_codec_dev_write(codec_dev, (void *)data, length);

    return ret;
}

esp_err_t bsp_get_feed_data(bool is_get_raw_channel, int16_t *buffer, int buffer_len)
{
    esp_err_t ret = ESP_OK;
    if (!codec_dev) {
        return ESP_FAIL;
    }

    ret = esp_codec_dev_read(codec_dev, (void *)buffer, buffer_len);

    return ret;
}

int bsp_get_feed_channel(void)
{
    return ADC_I2S_CHANNEL;
}

char* bsp_get_input_format(void)
{
    return "MR";
}

static void bsp_enable_audio_board_power(void)
{
    // Turn on the power for audio board, so it can go from "No Power" state to "Shutdown" state
    if (!esp_ldo_enabled) {
        esp_ldo_channel_config_t ldo_audio_board_config = {
            .chan_id = LDO_CHANNEL_ID,
            .voltage_mv = LDO_CFG_VOL_MV,
        };
        ESP_ERROR_CHECK(esp_ldo_acquire_channel(&ldo_audio_board_config, &ldo_audio_board));
        esp_ldo_enabled = true;
    }
    ESP_LOGI(TAG, "Audio board powered on");
}

static void bsp_disable_audio_board_power(void)
{
    if (ldo_audio_board) {
        esp_ldo_release_channel(ldo_audio_board);
        ldo_audio_board = NULL;
        esp_ldo_enabled = false;
    }
}

esp_err_t bsp_board_init(uint32_t sample_rate, int channel_format, int bits_per_chan)
{
    // Turn on the power for audio board
    // bsp_enable_audio_board_power();

    /*!< Initialize I2C bus, used for audio codec*/
    bsp_i2c_init(I2C_NUM, I2C_CLK);
    
    if (sample_rate != 16000) {
       ESP_LOGE(TAG, "Unable to configure sample_rate. It's only support 16000."); 
       sample_rate = 16000;
    }
    s_play_sample_rate = sample_rate;

    if (channel_format != 2) {
        ESP_LOGE(TAG, "Unable to configure channel_format. It's only support 2.");
        channel_format = 2;
    }
    s_play_channel_format = channel_format;

    if (bits_per_chan != 16) {
        ESP_LOGE(TAG, "Unable to configure bits_per_chan. It's only support 16.");
        bits_per_chan = 16;
    }
    s_bits_per_chan = bits_per_chan;

    bsp_i2s_init(I2S_NUM_1, 16000, 2, 16);
    // Because record and play use the same i2s.
    bsp_codec_init(16000, 16000, 2, 16);
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
     * On these chips, the SDMMC IO power is supplied externally
     */
    if (!esp_ldo_enabled) {
        sd_pwr_ctrl_ldo_config_t ldo_config = {
            .ldo_chan_id = LDO_CHANNEL_ID, // `LDO_VO4` is used as the SDMMC IO power
        };
        ret_val = sd_pwr_ctrl_new_on_chip_ldo(&ldo_config, &pwr_ctrl_handle);
        if (ret_val != ESP_OK) {
            ESP_LOGE(TAG, "Failed to new an on-chip ldo power control driver");
            return ret_val;
        }
        // pwr_ctrl_handle->set_io_voltage(pwr_ctrl_handle->ctx, LDO_CFG_VOL_MV);
        host.pwr_ctrl_handle = pwr_ctrl_handle;
        esp_ldo_enabled = true;
    }

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

    if (!pwr_ctrl_handle) {
        ret_val = sd_pwr_ctrl_del_on_chip_ldo(pwr_ctrl_handle);
        if (ret_val != ESP_OK) {
            ESP_LOGE(TAG, "Failed to delete on-chip ldo power control driver");
        }
        pwr_ctrl_handle = NULL;
        esp_ldo_enabled = false;
    }

    return ret_val;
}
