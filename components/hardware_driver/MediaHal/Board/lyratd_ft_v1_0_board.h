/*
 * ESPRESSIF MIT License
 *
 * Copyright (c) 2018 <ESPRESSIF SYSTEMS (SHANGHAI) PTE LTD>
 *
 * Permission is hereby granted for use on all ESPRESSIF SYSTEMS products, in which case,
 * it is free of charge, to any person obtaining a copy of this software and associated
 * documentation files (the "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the Software is furnished
 * to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all copies or
 * substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
 * COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 * IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 */

#ifndef _AUDIO_LYRATD_FT_V1_0_BOARD_H_
#define _AUDIO_LYRATD_FT_V1_0_BOARD_H_

#ifdef __cplusplus
extern "C" {
#endif
/* Select System Functions */
#define BOARD_INFO                  "ESP_LYRATD_FT_V1_0"

#define SD_CARD_OPEN_FILE_NUM_MAX   5
#define ENABLE_ADC_BUTTON
#define ENABLE_KEYWORD_DETECT
// #define ENABLE_BATTERY_ADC

/* SD card related */
#define SD_CARD_INTR_GPIO           34

#define SD_CARD_DATA0               2
#define SD_CARD_DATA1               4
// #define SD_CARD_DATA2               12
// #define SD_CARD_DATA3               13
#define SD_CARD_CMD                 15
#define SD_CARD_CLK                 14

/* Adc button pin */
#ifdef ENABLE_ADC_BUTTON
#define BUTTON_ADC_CH             ADC1_CHANNEL_3 // GPIO 39
#endif

/* Battery detect pin */
#ifdef ENABLE_POWER_VOLTAGE_DETECT
#define PWR_VOLTAGE_ADC_CH
#endif

/* I2C gpios */

/* DSP Chip ForteMedia IM501 */
#define DSP_IRQ_DETECT_PIN          36
#define DSP_RESET_CTRL_PIN          21

#define DSP_RECV_I2S_BCK            32
#define DSP_RECV_I2S_LRCK           33
#define DSP_RECV_I2S_DATA           35

#define DSP_FT_SPI_CS               12
#define DSP_FT_SPI_CLK              13
#define DSP_FT_SPI_MOSI             19
#define DSP_FT_SPI_MISO             27

/* PA */
#define GPIO_PA_EN                  22

#define GPIO_LED_GREEN              23
#define GPIO_LED_RED                18

// I2C for DSP not used
#define IIC_CLK                     23
#define IIC_DATA                    18

/* Playback I2S gpios */
#define IIS_MCLK                    0
#define IIS_SCLK                    5
#define IIS_LCLK                    25
#define IIS_DSIN                    26
#define IIS_DOUT                    -1

#ifdef __cplusplus
}
#endif

#endif
