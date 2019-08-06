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
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "esp_system.h"

#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "freertos/queue.h"
#include "freertos/event_groups.h"
#include "driver/i2s.h"

#include "board.h"
#include "DspI2c.h"
#include "im501_driver.h"
#include "im501_SPI_driver.h"

#include "recorder_engine.h"

#if defined CONFIG_ESP_LYRATD_FT_V1_0_BOARD || defined CONFIG_ESP_LYRATD_FT_DOSS_V1_0_BOARD

#define DSP_SER_TAG                     "DSP_SER"
#define DSP_SERV_TASK_PRIORITY          3
#define DSP_SERV_TASK_STACK_SIZE        2700
#define I2S1_NUM                        1  //only i2s0 support PDM and ADC
#define DSP_RESET_CTRL_PIN_SEL          ((1ULL)<<DSP_RESET_CTRL_PIN)

int I2s1_init(void)
{
    int res = 0;
    i2s_config_t i2s_config = {
        .mode = I2S_MODE_MASTER | I2S_MODE_RX,//the mode must be set according to DSP configuration
        .sample_rate = 16000,                           //must be the same as DSP configuration
        .channel_format = I2S_CHANNEL_FMT_RIGHT_LEFT,   //must be the same as DSP configuration
        .bits_per_sample = 16,                          //must be the same as DSP configuration
        .communication_format = I2S_COMM_FORMAT_I2S,
        .dma_buf_count = 3,                            /*!< amount of the dam buffer sectors*/
        .dma_buf_len = 300,                            /*!< dam buffer size of each sector (word, i.e. 4 Bytes) */
        .intr_alloc_flags = ESP_INTR_FLAG_LEVEL2,
    };
    i2s_pin_config_t pin_config = {
        .bck_io_num = DSP_RECV_I2S_BCK,
        .ws_io_num = DSP_RECV_I2S_LRCK,
        .data_out_num = -1,
        .data_in_num = DSP_RECV_I2S_DATA
    };
    res = i2s_driver_install(I2S1_NUM, &i2s_config, 0, NULL);

    if (res) {
        return res;
    }

    res |= i2s_set_pin(I2S1_NUM, &pin_config);
    i2s_zero_dma_buffer(I2S1_NUM);

    return res;
}

int im501_gpio_init()
{
    int res = 0;
    gpio_config_t  io_conf;
    memset(&io_conf, 0, sizeof(io_conf));
    io_conf.intr_type = GPIO_PIN_INTR_DISABLE;
    io_conf.mode = GPIO_MODE_OUTPUT;
    io_conf.pin_bit_mask = DSP_RESET_CTRL_PIN_SEL;
    io_conf.pull_down_en = 0;
    io_conf.pull_up_en = 0;
    res |= gpio_config(&io_conf);
    res |= gpio_set_level(DSP_RESET_CTRL_PIN, 0);
    vTaskDelay(100 / portTICK_RATE_MS);
    res |= gpio_set_level(DSP_RESET_CTRL_PIN, 1);
    return res;
}

int init_forte_dsp()
{
    int res  = 0;
    res |= I2s1_init();     //Must be called before Forte dsp initialized due to the mclk need to output first.
    res |= im501_gpio_init();
    res |= initial_im501();

    /* DSP */
    // xTaskCreate(im501_int, "im501_int", DSP_SERV_TASK_STACK_SIZE, NULL, 1, NULL);
    return res;
}

#endif
