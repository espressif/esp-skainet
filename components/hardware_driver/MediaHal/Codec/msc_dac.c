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

#include "userconfig.h"

#if defined CONFIG_CODEC_CHIP_IS_MICROSEMI
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "vprocTwolf_access.h"

#define MSC_I2S_NUM 0
static const char *TAG = "MSC";

int8_t tw_vol[15] = { -90, -45, -30, -23, -16, -10, -4, -1, 0, 1, 2, 3, 4, 5, 6};

int msc_dac_init(void *cfg)
{
    int ret  = 0;
    // Set I2S pins
    // ret = i2s_driver_install(MSC_I2S_NUM, &i2s_config, 0, NULL);
    // ret |= i2s_set_pin(MSC_I2S_NUM, &i2s_pin); // I2S0 default

    return ESP_OK;
}

int msc_dac_uninit(void)
{
    return ESP_OK;
}

int msc_dac_start(int mode)
{
    int ret = 0;

    return ret;
}

int msc_dac_stop(int mode)
{
    return ESP_OK;
}

int msc_dac_config_fmt(int mode, int fmt)
{
    return ESP_OK;
}
int msc_dac_set_bits(int mode, int bitPerSample)
{
    return ESP_OK;
}
int msc_adc_input(int input)
{
    return ESP_OK;
}

int msc_dac_set_volume(int volume)
{
    int ret = 0;
    if (volume < 0 ) {
        volume = 0;
    } else if (volume >= 100) {
        volume = 100;
    }
    int k = volume / 7;
    ESP_LOGI(TAG, "set vol %d,k:%d, %d", volume, k, tw_vol[k]);
    ret = VprocTwolfSetVolume(tw_vol[k]);
    return ret;
}

int msc_dac_get_volume(int *volume)
{
    int ret = 0;
    int8_t vol = 0;
    ret = VprocTwolfGetVolume(&vol);
    *volume = 0;
    for (int i = 0; i < sizeof(tw_vol); ++i) {
        if (vol == tw_vol[i]) {
            *volume = i * 7;
        }
    }
    ESP_LOGI(TAG, "get vol %d", (int)*volume);
    return ret;
}

int msc_dac_set_mute(int mute)
{
    return ESP_OK;
}

int msc_dac_get_mute(int *mute)
{
    return ESP_OK;
}
#endif