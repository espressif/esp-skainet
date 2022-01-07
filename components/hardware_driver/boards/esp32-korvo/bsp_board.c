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
#include "driver/i2s.h"
#include "driver/gpio.h"
#include "esp_err.h"
#include "esp_log.h"
#include "i2c_bus.h"
#include "esp_rom_sys.h"
#include "bsp_board.h"
#include "es7210.h"
#include "es8311.h"
#include "esp_vfs_fat.h"
#include "sdmmc_cmd.h"
#if ((SOC_SDMMC_HOST_SUPPORTED) && (FUNC_SDMMC_EN))
#include "driver/sdmmc_host.h"
#endif /* ((SOC_SDMMC_HOST_SUPPORTED) && (FUNC_SDMMC_EN)) */

#define GPIO_MUTE_NUM   GPIO_NUM_1
#define GPIO_MUTE_LEVEL 1
#define ACK_CHECK_EN   0x1     /*!< I2C master will check ack from slave*/
#define ADC_I2S_CHANNEL 4
static sdmmc_card_t *card;
static i2c_bus_handle_t i2c_bus_handle = NULL;
static const char *TAG = "board";
static int s_play_sample_rate = 16000;
static int s_play_channel_format = 1;
static int s_bits_per_chan = 16;
typedef enum {
    CODEC_TYPE_ES7210 = 0,
    CODEC_TYPE_ES8311,
    CODEC_TYPE_ES8388,
    CODEC_TYPE_MAX,
    CODEC_TYPE_NONE = -1,
} codec_type_t;

typedef struct {
    uint8_t dev_addr;
    char *dev_name;
    codec_type_t dev_type;
} codec_dev_t;

static codec_dev_t codec_dev_list[] = {
    { 0x40, "ES7210", CODEC_TYPE_ES7210 },
    { 0x18, "ES8311", CODEC_TYPE_ES8311 },
    { 0x20, "ES8388", CODEC_TYPE_ES8388 },
};

esp_err_t bsp_i2c_add_device(i2c_bus_device_handle_t *i2c_device_handle, uint8_t dev_addr)
{
    if (NULL == i2c_bus_handle) {
        ESP_LOGE(TAG, "Failed create I2C device");
        return ESP_FAIL;
    }

    *i2c_device_handle = i2c_bus_device_create(i2c_bus_handle, dev_addr, 400000);

    if (NULL == i2c_device_handle) {
        ESP_LOGE(TAG, "Failed create I2C device");
        return ESP_FAIL;
    }

    return ESP_OK;
}

i2c_bus_handle_t bsp_i2c_bus_get_handle(void)
{
    return i2c_bus_handle;
}

esp_err_t bsp_i2c_init(i2c_port_t i2c_num, uint32_t clk_speed)
{
    /* Check if bus is already created */
    if (NULL != i2c_bus_handle) {
        ESP_LOGE(TAG, "I2C bus already initialized.");
        return ESP_FAIL;
    }

    i2c_config_t conf = {
        .mode = I2C_MODE_MASTER,
        .scl_io_num = GPIO_I2C_SCL,
        .sda_io_num = GPIO_I2C_SDA,
        .scl_pullup_en = GPIO_PULLUP_ENABLE,
        .sda_pullup_en = GPIO_PULLUP_ENABLE,
        .master.clk_speed = clk_speed,
    };

    i2c_bus_handle = i2c_bus_create(i2c_num, &conf);
    
    if (NULL == i2c_bus_handle) {
        ESP_LOGE(TAG, "Failed create I2C bus");
        return ESP_FAIL;
    }

    return ESP_OK;
}

esp_err_t bsp_i2c_probe(void)
{
    if (NULL == bsp_i2c_bus_get_handle()) {
        ESP_LOGE(TAG, "I2C bus not initialized");
        return ESP_ERR_INVALID_STATE;
    }

    i2c_bus_t *i2c_bus = (i2c_bus_t *) bsp_i2c_bus_get_handle();

    for (size_t i = 1; i < 0x80; i++) {
        i2c_cmd_handle_t cmd = i2c_cmd_link_create();
        i2c_master_start(cmd);
        i2c_master_write_byte(cmd, ( i << 1 ), ACK_CHECK_EN);
        i2c_master_stop(cmd);
        esp_err_t ret_val = i2c_master_cmd_begin(i2c_bus->i2c_port, cmd, pdMS_TO_TICKS(500));
        i2c_cmd_link_delete(cmd);
        if(ESP_OK == ret_val) {
            ESP_LOGW(TAG, "Found I2C Device at 0x%02X", i);
        }
    }

    return ESP_OK;
}

esp_err_t bsp_i2c_probe_addr(uint8_t addr)
{
    /* Use 7 bit address here */
    if (addr >= 0x80) {
        return ESP_ERR_INVALID_ARG;
    }

    /* Check if I2C bus initialized */
    if (NULL == bsp_i2c_bus_get_handle()) {
        ESP_LOGE(TAG, "I2C bus not initialized");
        return ESP_ERR_INVALID_STATE;
    }

    /* Get I2C bus object from i2c_bus_handle */
    i2c_bus_t *i2c_bus = (i2c_bus_t *) bsp_i2c_bus_get_handle();

    /* Create probe cmd link */
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, ( addr << 1 ), ACK_CHECK_EN);
    i2c_master_stop(cmd);

    /* Start probe cmd link */
    esp_err_t ret_val = i2c_master_cmd_begin(i2c_bus->i2c_port, cmd, pdMS_TO_TICKS(500));

    /* Delete cmd link after probe ends */
    i2c_cmd_link_delete(cmd);

    /* Get probe result if ESP_OK equals to ret_val */
    return ret_val;
}

esp_err_t bsp_codec_adc_init(audio_hal_iface_samples_t sample_rate)
{
    esp_err_t ret_val = ESP_OK;
    audio_hal_codec_config_t cfg = {
        .codec_mode = AUDIO_HAL_CODEC_MODE_ENCODE,
        .adc_input = AUDIO_HAL_ADC_INPUT_ALL,
        .i2s_iface = {
            .bits = AUDIO_HAL_BIT_LENGTH_16BITS,
            .fmt = AUDIO_HAL_I2S_NORMAL,
            .mode = AUDIO_HAL_MODE_SLAVE,
            .samples = sample_rate,
        },
    };

    ret_val |= es7210_adc_init(&cfg);
    ret_val |= es7210_adc_config_i2s(cfg.codec_mode, &cfg.i2s_iface);
    ret_val |= es7210_adc_set_gain(ES7210_INPUT_MIC1, GAIN_33DB);
    ret_val |= es7210_adc_set_gain(ES7210_INPUT_MIC2, GAIN_33DB);
    ret_val |= es7210_adc_set_gain(ES7210_INPUT_MIC3, GAIN_33DB);
    ret_val |= es7210_adc_set_gain(ES7210_INPUT_MIC4, GAIN_33DB);
    ret_val |= es7210_adc_ctrl_state(cfg.codec_mode, AUDIO_HAL_CTRL_START);

    if (ESP_OK != ret_val) {
        ESP_LOGE(TAG, "Failed initialize codec");
    }

    return ret_val;
}

esp_err_t bsp_codec_dac_init(audio_hal_iface_samples_t sample_rate)
{
    esp_err_t ret_val = ESP_OK;
    audio_hal_codec_config_t cfg = {
        .codec_mode = AUDIO_HAL_CODEC_MODE_DECODE,
        .dac_output = AUDIO_HAL_DAC_OUTPUT_LINE1,
        .i2s_iface = {
            .bits = AUDIO_HAL_BIT_LENGTH_16BITS,
            .fmt = AUDIO_HAL_I2S_NORMAL,
            .mode = AUDIO_HAL_MODE_SLAVE,
            .samples = sample_rate,
        },
    };

    ret_val |= es8311_codec_init(&cfg);
    ret_val |= es8311_set_bits_per_sample(cfg.i2s_iface.bits);
    ret_val |= es8311_config_fmt(cfg.i2s_iface.fmt);
    ret_val |= es8311_codec_set_voice_volume(60);
    ret_val |= es8311_codec_ctrl_state(cfg.codec_mode, AUDIO_HAL_CTRL_START);
    ret_val |= es8311_codec_set_clk();

    if (ESP_OK != ret_val) {
        ESP_LOGE(TAG, "Failed initialize codec");
    }

    return ret_val;
}

static esp_err_t bsp_i2s_init(i2s_port_t i2s_num, uint32_t sample_rate, i2s_channel_fmt_t channel_format, i2s_bits_per_chan_t bits_per_chan)
{
    esp_err_t ret_val = ESP_OK;

    if (i2s_num == I2S_NUM_1) {
        i2s_config_t i2s_config = I2S_CONFIG_DEFAULT();

        i2s_pin_config_t pin_config = {
            .bck_io_num = GPIO_I2S_SCLK,
            .ws_io_num = GPIO_I2S_LRCK,
            .data_out_num = GPIO_I2S_DOUT,
            .data_in_num = GPIO_I2S_SDIN,
            .mck_io_num = GPIO_I2S_MCLK,
        };

        ret_val |= i2s_driver_install(i2s_num, &i2s_config, 0, NULL);
        ret_val |= i2s_set_pin(i2s_num, &pin_config);
    } else if (i2s_num == I2S_NUM_0) {
        i2s_config_t i2s_config = I2S0_CONFIG_DEFAULT();

        i2s_pin_config_t pin_config = {
            .bck_io_num = GPIO_I2S0_SCLK,
            .ws_io_num = GPIO_I2S0_LRCK,
            .data_out_num = GPIO_I2S0_DOUT,
            .data_in_num = GPIO_I2S0_SDIN,
            .mck_io_num = GPIO_I2S0_MCLK,
        };

        ret_val |= i2s_driver_install(i2s_num, &i2s_config, 0, NULL);
        ret_val |= i2s_set_pin(i2s_num, &pin_config);
    }


    return ret_val;
}

static esp_err_t bsp_i2s_deinit(i2s_port_t i2s_num)
{
    esp_err_t ret_val = ESP_OK;

    ret_val |= i2s_stop(I2S_NUM_0);
    ret_val |= i2s_driver_uninstall(i2s_num);

    return ret_val;
}

static esp_err_t bsp_codec_prob(int *codec_type)
{
    for (size_t i = 0; i < sizeof(codec_dev_list) / sizeof(codec_dev_list[0]); i++) {
        if (ESP_OK == bsp_i2c_probe_addr(codec_dev_list[i].dev_addr)) {
            *codec_type |= 1 << i;
            ESP_LOGI(TAG, "Detected codec at 0x%02X. Name : %s",
                codec_dev_list[i].dev_addr, codec_dev_list[i].dev_name);
        }
    }

    if (0 == *codec_type) {
        *codec_type = CODEC_TYPE_NONE;
        ESP_LOGW(TAG, "Codec not detected");
        return ESP_ERR_NOT_FOUND;
    }

    return ESP_OK;
}

static esp_err_t bsp_codec_init(audio_hal_iface_samples_t adc_sample_rate, audio_hal_iface_samples_t dac_sample_rate)
{
    esp_err_t ret_val = ESP_OK;

    static int codec_type = 0;

    ret_val |= bsp_codec_prob(&codec_type);

    if (CODEC_TYPE_NONE == codec_type) {
        return ESP_ERR_NOT_FOUND;
    }

    if (((1 << CODEC_TYPE_ES8311) + (1 << CODEC_TYPE_ES7210)) == codec_type) {
        ret_val |= bsp_codec_adc_init(adc_sample_rate);
        ret_val |= bsp_codec_dac_init(dac_sample_rate);
        return ret_val;
    }

    ESP_LOGW(TAG, "Currently not support");
    return ESP_ERR_NOT_SUPPORTED;
}

esp_err_t bsp_audio_play(const int16_t* data, int length, TickType_t ticks_to_wait)
{
    size_t bytes_write = 0;
    esp_err_t ret = ESP_OK;
    ret = i2s_write(I2S_NUM_0, (const char*) data, length, &bytes_write, ticks_to_wait);

    return ret;
}

esp_err_t bsp_get_feed_data(int16_t *buffer, int buffer_len)
{
    esp_err_t ret = ESP_OK;
    size_t bytes_read;
    int audio_chunksize = buffer_len / (sizeof(int16_t) * ADC_I2S_CHANNEL);
    ret = i2s_read(I2S_NUM_1, buffer, buffer_len, &bytes_read, portMAX_DELAY);

    for (int i = 0; i < audio_chunksize; i++) {
        int16_t ref = buffer[4 * i + 0];
        buffer[2 * i + 0] = buffer[4 * i + 1];
        buffer[2 * i + 1] = ref;
    }

    return ret;
}

int bsp_get_feed_channel(void)
{
    return ADC_I2S_CHANNEL;
}

esp_err_t bsp_board_init(audio_hal_iface_samples_t sample_rate, int channel_format, int bits_per_chan)
{
    int sample_fre = 16000;
    i2s_channel_fmt_t channel_fmt = I2S_CHANNEL_FMT_RIGHT_LEFT;
    /*!< Initialize I2C bus, used for audio codec*/
    bsp_i2c_init(I2C_NUM_0, 400 * 1000);
    switch (sample_rate) {
    case AUDIO_HAL_08K_SAMPLES:
        sample_fre = 8000;
        break;
    case AUDIO_HAL_11K_SAMPLES:
        sample_fre = 11025;
        break;
    case AUDIO_HAL_16K_SAMPLES:
        sample_fre = 16000;
        break;
    case AUDIO_HAL_22K_SAMPLES:
        sample_fre = 22050;
        break;
    case AUDIO_HAL_24K_SAMPLES:
        sample_fre = 24000;
        break;
    case AUDIO_HAL_32K_SAMPLES:
        sample_fre = 32000;
        break;
    case AUDIO_HAL_44K_SAMPLES:
        sample_fre = 44100;
        break;
    case AUDIO_HAL_48K_SAMPLES:
        sample_fre = 48000;
        break;
    default:
        ESP_LOGE(TAG, "Unable to configure sample rate %dHz", sample_fre);
        break;
    }

    if (channel_format == 1) {
        channel_fmt = I2S_CHANNEL_FMT_ONLY_LEFT;
    } else if (channel_format == 2) {
        channel_fmt = I2S_CHANNEL_FMT_RIGHT_LEFT;
    } else {
        ESP_LOGE(TAG, "Unable to configure channel_format %d", channel_format);
        channel_format = 1;
        channel_fmt = I2S_CHANNEL_FMT_ONLY_LEFT;
    }

    if (bits_per_chan != 16 && bits_per_chan != 32) {
        ESP_LOGE(TAG, "Unable to configure bits_per_chan %d", bits_per_chan);
        bits_per_chan = 16;
    }


    bsp_i2s_init(I2S_NUM_1, 16000, I2S_CHANNEL_FMT_RIGHT_LEFT, I2S_BITS_PER_CHAN_32BIT);

    s_play_sample_rate = sample_fre;
    s_play_channel_format = channel_format;
    s_bits_per_chan = bits_per_chan;    
    bsp_i2s_init(I2S_NUM_0, sample_fre, channel_fmt, bits_per_chan);

    bsp_codec_init(AUDIO_HAL_16K_SAMPLES, sample_rate);
    /* Initialize PA */
    gpio_config_t  io_conf;
    memset(&io_conf, 0, sizeof(io_conf));
    io_conf.intr_type = GPIO_PIN_INTR_DISABLE;
    io_conf.mode = GPIO_MODE_OUTPUT;
    io_conf.pin_bit_mask = ((1ULL << GPIO_PWR_CTRL));
    io_conf.pull_down_en = 0;
    io_conf.pull_up_en = 0;
    gpio_config(&io_conf);
    gpio_set_level(GPIO_PWR_CTRL, 1);
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