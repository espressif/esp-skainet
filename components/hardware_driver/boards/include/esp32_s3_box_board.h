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
#pragma once

#include "driver/gpio.h"
#include "esp_idf_version.h"
#include "esp_codec_dev.h"
#include "esp_codec_dev_defaults.h"
#include "esp_codec_dev_os.h"

/**
 * @brief ESP32-S3-BOX I2C GPIO defineation
 * 
 */
#define FUNC_I2C_EN     (1)
#define I2C_NUM         (0)
#define I2C_CLK         (600000)
#define GPIO_I2C_SCL    (GPIO_NUM_18)
#define GPIO_I2C_SDA    (GPIO_NUM_8)

/**
 * @brief ESP32-S3-BOX SDMMC GPIO defination
 * 
 * @note Only avaliable when PMOD connected
 */
#define FUNC_SDMMC_EN   (1)
#define SDMMC_BUS_WIDTH (4)
#define GPIO_SDMMC_CLK  (GPIO_NUM_13)
#define GPIO_SDMMC_CMD  (GPIO_NUM_11)
#define GPIO_SDMMC_D0   (GPIO_NUM_14)
#define GPIO_SDMMC_D1   (GPIO_NUM_12)
#define GPIO_SDMMC_D2   (GPIO_NUM_10)
#define GPIO_SDMMC_D3   (GPIO_NUM_9)
#define GPIO_SDMMC_DET  (GPIO_NUM_NC)

/**
 * @brief ESP32-S3-BOX SDSPI GPIO definationv
 * 
 */
#define FUNC_SDSPI_EN       (0)
#define SDSPI_HOST          (SPI2_HOST)
#define GPIO_SDSPI_CS       (GPIO_NUM_NC)
#define GPIO_SDSPI_SCLK     (GPIO_NUM_NC)
#define GPIO_SDSPI_MISO     (GPIO_NUM_NC)
#define GPIO_SDSPI_MOSI     (GPIO_NUM_NC)

/**
 * @brief ESP32-S3-BOX I2S GPIO defination
 * 
 */
#define FUNC_I2S_EN         (1)
#define GPIO_I2S_LRCK       (GPIO_NUM_47)
#define GPIO_I2S_MCLK       (GPIO_NUM_2)
#define GPIO_I2S_SCLK       (GPIO_NUM_17)
#define GPIO_I2S_SDIN       (GPIO_NUM_16)
#define GPIO_I2S_DOUT       (GPIO_NUM_15)

/**
 * @brief ESP32-S3-BOX I2S GPIO defination
 * 
 */
#define FUNC_I2S0_EN         (0)
#define GPIO_I2S0_LRCK       (GPIO_NUM_NC)
#define GPIO_I2S0_MCLK       (GPIO_NUM_NC)
#define GPIO_I2S0_SCLK       (GPIO_NUM_NC)
#define GPIO_I2S0_SDIN       (GPIO_NUM_NC)
#define GPIO_I2S0_DOUT       (GPIO_NUM_NC)

#define RECORD_VOLUME   (27.0)
/**
 * @brief player configurations
 *
 */
#define PLAYER_VOLUME   (50)

/**
 * @brief ESP32-S3-HMI-DevKit power control IO
 * 
 * @note Some power control pins might not be listed yet
 * 
 */
#define FUNC_PWR_CTRL       (1)
#define GPIO_PWR_CTRL       (GPIO_NUM_46)
#define GPIO_PWR_ON_LEVEL   (1)

#if ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(5, 0, 0)

#define I2S_CONFIG_DEFAULT(sample_rate, channel_fmt, bits_per_chan) { \
        .clk_cfg  = I2S_STD_CLK_DEFAULT_CONFIG(16000), \
        .slot_cfg = I2S_STD_PHILIPS_SLOT_DEFAULT_CONFIG(32, I2S_SLOT_MODE_STEREO), \
        .gpio_cfg = { \
            .mclk = GPIO_I2S_MCLK, \
            .bclk = GPIO_I2S_SCLK, \
            .ws   = GPIO_I2S_LRCK, \
            .dout = GPIO_I2S_DOUT, \
            .din  = GPIO_I2S_SDIN, \
        }, \
    }

#else

#define I2S_CONFIG_DEFAULT(sample_rate, channel_fmt, bits_per_chan) { \
    .mode                   = I2S_MODE_MASTER | I2S_MODE_RX | I2S_MODE_TX, \
    .sample_rate            = 16000, \
    .bits_per_sample        = I2S_BITS_PER_SAMPLE_32BIT, \
    .channel_format         = I2S_CHANNEL_FMT_RIGHT_LEFT, \
    .communication_format   = I2S_COMM_FORMAT_STAND_I2S, \
    .intr_alloc_flags       = ESP_INTR_FLAG_LEVEL1, \
    .dma_buf_count          = 6, \
    .dma_buf_len            = 160, \
    .use_apll               = false, \
    .tx_desc_auto_clear     = true, \
    .fixed_mclk             = 0, \
    .mclk_multiple          = I2S_MCLK_MULTIPLE_DEFAULT, \
    .bits_per_chan          = I2S_BITS_PER_CHAN_32BIT, \
}

#endif
