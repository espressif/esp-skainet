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

#include <string.h>
#include "esp_log.h"
#include "ES7149_interface.h"
#include "userconfig.h"

#define Es7149_TAG "7149"

#define ES_ASSERT(a, format, b, ...) \
    if ((a) != 0) { \
        ESP_LOGE(Es7149_TAG, format, ##__VA_ARGS__); \
        return b;\
    }

#define LOG_7149(fmt, ...)   ESP_LOGW(Es7149_TAG, fmt, ##__VA_ARGS__)
static int es7149_vol;

/**
 * @brief Configure Es7149 ADC and DAC volume. Basicly you can consider this as ADC and DAC gain
 *
 * @param mode:             set ADC or DAC or all
 * @param volume:           -96 ~ 0              for example Es7149SetAdcDacVolume(ES_MODULE_ADC, 30, 6); means set ADC volume -30.5db
 * @param dot:              whether include 0.5. for example Es7149SetAdcDacVolume(ES_MODULE_ADC, 30, 4); means set ADC volume -30db
 *
 * @return
 *     - (-1) Parameter error
 *     - (0)   Success
 */
// static int Es7149SetAdcDacVolume(int mode, int volume, int dot)
// {
//     int res = 0;
//     return res;
// }

/**
 * @brief Power Management
 *
 * @param mod:      if ES_POWER_CHIP, the whole chip including ADC and DAC is enabled
 * @param enable:   false to disable true to enable
 *
 * @return
 *     - (-1)  Error
 *     - (0)   Success
 */
int Es7149Start(ESCodecModule mode)
{
    int res = 0;
    return res;
}
/**
 * @brief Power Management
 *
 * @param mod:      if ES_POWER_CHIP, the whole chip including ADC and DAC is enabled
 * @param enable:   false to disable true to enable
 *
 * @return
 *     - (-1)  Error
 *     - (0)   Success
 */
int Es7149Stop(ESCodecModule mode)
{
    int res = 0;
    return res;
}


/**
 * @brief Config I2s clock in MSATER mode
 *
 * @param cfg.sclkDiv:      generate SCLK by dividing MCLK in MSATER mode
 * @param cfg.lclkDiv:      generate LCLK by dividing MCLK in MSATER mode
 *
 * @return
 *     - (-1)  Error
 *     - (0)   Success
 */
int Es7149I2sConfigClock(ESCodecI2sClock cfg)
{
    int res = 0;
    return res;
}

void Es7149Uninit()
{
}

/**
 * @return
 *     - (-1)  Error
 *     - (0)   Success
 */
int Es7149Init(Es7149Config *cfg)
{
    int res = 0;
    return res;
}

/**
 * @brief Configure Es7149 I2S format
 *
 * @param mode:           set ADC or DAC or all
 * @param bitPerSample:   see Es7149I2sFmt
 *
 * @return
 *     - (-1) Error
 *     - (0)  Success
 */
int Es7149ConfigFmt(ESCodecModule mode, ESCodecI2SFmt fmt)
{
    int res = 0;
    return res;
}

/**
 * @param volume: 0 ~ 100
 *
 * @return
 *     - (-1)  Error
 *     - (0)   Success
 */
int Es7149SetVoiceVolume(int volume)
{
    int res = 0;
    es7149_vol = volume;
    return res;
}
/**
 *
 * @return
 *           volume
 */
int Es7149GetVoiceVolume(int *volume)
{
    int res = 0;
    *volume = es7149_vol;
    return res;
}

/**
 * @brief Configure Es7149 data sample bits
 *
 * @param mode:             set ADC or DAC or all
 * @param bitPerSample:   see BitsLength
 *
 * @return
 *     - (-1) Parameter error
 *     - (0)   Success
 */
int Es7149SetBitsPerSample(ESCodecModule mode, BitsLength bitPerSample)
{
    int res = 0;
    return res;
}

/**
 * @brief Configure Es7149 DAC mute or not. Basicly you can use this function to mute the output or don't
 *
 * @param enable: enable or disable
 *
 * @return
 *     - (-1) Parameter error
 *     - (0)   Success
 */
int Es7149SetVoiceMute(int enable)
{
    int res = 0;
    return res;
}

int Es7149GetVoiceMute(int *mute)
{
    int res = -1;
    return res;
}

/**
 * @param gain: Config ADC input
 *
 * @return
 *     - (-1) Parameter error
 *     - (0)   Success
 */
int Es7149ConfigAdcInput(AdcInput input)
{
    int res = 0;
    return res;
}

/**
 * @param gain: see MicGain
 *
 * @return
 *     - (-1) Parameter error
 *     - (0)   Success
 */
int Es7149SetMicGain(MicGain gain)
{
    int res = 0;
    return res;
}

void Es7149ReadAll()
{

}

