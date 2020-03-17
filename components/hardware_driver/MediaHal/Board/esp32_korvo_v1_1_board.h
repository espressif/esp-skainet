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

#ifndef _AUDIO_KORVO_V1_1_BOARD_H_
#define _AUDIO_KORVO_V1_1_BOARD_H_

#ifdef __cplusplus
extern "C" {
#endif

/* Board functions related */
#define DIFFERENTIAL_MIC            0
#define BOARD_INFO                  "ESP32_KORVO_V1_1"
#define ENABLE_AUXIN_DETECT
#define ENABLE_KEYWORD_DETECT
#define ENABLE_ADC_BUTTON
#define ENABLE_MCLK_GPIO0
/* SD card related */
#define SD_CARD_INTR_GPIO           34
#define SD_CARD_INTR_SEL            GPIO_SEL_34
#define SD_CARD_PWR_CTRL            13	
#define SD_CARD_OPEN_FILE_NUM_MAX   5

#define SD_CARD_DATA0               2
#define SD_CARD_CMD                 15
#define SD_CARD_CLK                 14

/* AUX-IN related */
#ifdef ENABLE_AUXIN_DETECT
#define AUXIN_DETECT_PIN           19
#endif

/* Adc button pin */
#ifdef ENABLE_ADC_BUTTON
#define BUTTON_ADC_CH             ADC1_CHANNEL_3
#endif

/* LED indicators */
#define GPIO_LED_GREEN              22
#define GPIO_LED_BLUE               27

/* Headphone detect */
#define GPIO_HEADPHONE_DETECT       19

/* I2C gpios */
#define IIC_CLK                     32
#define IIC_DATA                    19

/* PA */
#define GPIO_PA_EN                  12

/* I2S gpios */
#define IIS_MCLK                    0
#define IIS_SCLK                    25
#define IIS_LCLK                    22
#define IIS_DSIN                    13
#define IIS_DOUT                    -1

/* I2S1 gpios */
#define IIS1_MCLK                   0
#define IIS1_SCLK                   27
#define IIS1_LCLK                   26
#define IIS1_DOUT                   36


#ifdef __cplusplus
}
#endif

#endif
