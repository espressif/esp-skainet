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
#include "driver/gpio.h"
#include "ES8388_interface.h"
#include "ES8374_interface.h"
#include "ES7149_interface.h"
#include "es8311.h"
#include "es7210.h"
#include "msc_dac.h"
#include "esp_log.h"
#include "MediaHal.h"
#include "driver/i2s.h"
#include "lock.h"
#include "InterruptionSal.h"

#define HAL_TAG "MEDIA_HAL"

#define MEDIA_HAL_CHECK_NULL(a, format, b, ...) \
    if ((a) == 0) { \
        ESP_LOGE(HAL_TAG, format, ##__VA_ARGS__); \
        return b;\
    }

#define I2S_OUT_VOL_DEFAULT     60
#define SUPPOERTED_BITS 16
#define I2S1_ENABLE     0   // Enable i2s1
#define I2S_DAC_EN      0  //if enabled then a speaker can be connected to i2s output gpio(GPIO25 and GND or GPIO26 and GND), using DAC(8bits) to play music
#define I2S_ADC_EN      0  //NOT SUPPORTED before idf 3.0. if enabled then a mic can be connected to i2s input gpio, using ADC to record 8bit-music

static int I2S_NUM = I2S_NUM_0;//only support 16 now and i2s0 or i2s1
static char MUSIC_BITS = 16; //for re-bits feature, but only for 16 to 32
static int AMPLIFIER = 1 << 8;//amplify the volume, fixed point

i2s_config_t i2s_config = {
#if I2S_DAC_EN == 1
    .mode = I2S_MODE_MASTER | I2S_MODE_TX | I2S_MODE_DAC_BUILT_IN,
//    #else if I2S_ADC_EN == 1 //NOT SUPPORTED before idf 3.0.
//    .mode = I2S_MODE_MASTER | I2S_MODE_RX | I2S_MODE_TX | I2S_MODE_ADC_BUILT_IN,
#else
    .mode = I2S_MODE_MASTER | I2S_MODE_RX | I2S_MODE_TX,
#endif
    .sample_rate = 16000,
    .bits_per_sample = SUPPOERTED_BITS,
#ifdef CONFIG_ESP_LYRAT_V4_3_BOARD
    .channel_format = I2S_CHANNEL_FMT_RIGHT_LEFT,
#else
    .channel_format = I2S_CHANNEL_FMT_ONLY_LEFT,
#endif
#if I2S_DAC_EN == 1
    .communication_format = I2S_COMM_FORMAT_I2S_MSB,
#else
    .communication_format = I2S_COMM_FORMAT_I2S,
#endif
    //when dma_buf_count = 3 and dma_buf_len = 300, then 3 * 4 * 300 * 2 Bytes internal RAM will be used. The multiplier 2 is for Rx buffer and Tx buffer together.
    .dma_buf_count = 3,                            /*!< amount of the dam buffer sectors*/
    .dma_buf_len = 300,                            /*!< dam buffer size of each sector (word, i.e. 4 Bytes) */
    .intr_alloc_flags = I2S_INTER_FLAG,
#if I2S_DAC_EN == 0
    .use_apll = 1,
#endif
};

const i2s_pin_config_t i2s_pin = {
    .bck_io_num = IIS_SCLK,
    .ws_io_num = IIS_LCLK,
    .data_out_num = IIS_DSIN,
    .data_in_num = IIS_DOUT
};

#if defined CONFIG_USE_ES7243 || defined CONFIG_USE_ES7210
const i2s_pin_config_t i2s1_pin = {
    .bck_io_num = IIS1_SCLK,
    .ws_io_num = IIS1_LCLK,
    .data_out_num = IIS_DSIN,
    .data_in_num = IIS1_DOUT
};
#endif

struct media_hal {
    //globle
    MediaHalState sMediaHalState;
    int amplifier_type;
    //private
    CodecMode _currentMode;
    xSemaphoreHandle _halLock;

    //functions
    int (*codec_init)(void *cfg);
    void (*codec_uninit)(void);
    int (*codec_sart)(int mode);
    int (*codec_stop)(int mode);
    int (*codec_config_fmt)(int mode, int fmt);
    int (*codec_set_bit)(int mode, int bitPerSample);
    int (*codec_set_adc_input)(int input);
    int (*codec_set_vol)(int volume);
    int (*codec_get_vol)(int *volume);
    int (*codec_set_mute)(int en);
    int (*codec_get_mute)(int *mute);
};


struct media_hal MediaHalConfig = {
    .sMediaHalState = 0,
    .amplifier_type = 0,
#if defined CONFIG_CODEC_CHIP_IS_ES8388
    .codec_init = Es8388Init,
    .codec_uninit = Es8388Uninit,
    .codec_sart = Es8388Start,
    .codec_stop = Es8388Stop,
    .codec_config_fmt = Es8388ConfigFmt,
    .codec_set_bit = Es8388SetBitsPerSample,
    .codec_set_adc_input = Es8388ConfigAdcInput,
    .codec_set_vol = Es8388SetVoiceVolume,
    .codec_get_vol = Es8388GetVoiceVolume,
    .codec_set_mute = Es8388SetVoiceMute,
    .codec_get_mute = Es8388GetVoiceMute,
#elif defined CONFIG_CODEC_CHIP_IS_ES8374
    .codec_init = Es8374Init,
    .codec_uninit = Es8374Uninit,
    .codec_sart = Es8374Start,
    .codec_stop = Es8374Stop,
    .codec_config_fmt = Es8374ConfigFmt,
    .codec_set_bit = Es8374SetBitsPerSample.
    .codec_set_adc_input = Es8374ConfigAdcInput,
    .codec_set_vol = Es8374SetVoiceVolume,
    .codec_get_vol = Es8374GetVoiceVolume,
    .codec_set_mute = Es8374SetVoiceMute,
    .codec_get_mute = Es8374GetVoiceMute,
#elif defined CONFIG_CODEC_CHIP_IS_ES7149
    .amplifier_type = 1,
    .codec_init = Es7149Init,
    .codec_uninit = Es7149Uninit,
    .codec_sart = Es7149Start,
    .codec_stop = Es7149Stop,
    .codec_config_fmt = Es7149ConfigFmt,
    .codec_set_bit = Es7149SetBitsPerSample,
    .codec_set_adc_input = Es7149ConfigAdcInput,
    .codec_set_vol = Es7149SetVoiceVolume,
    .codec_get_vol = Es7149GetVoiceVolume,
    .codec_set_mute = Es7149SetVoiceMute,
    .codec_get_mute = Es7149GetVoiceMute,
#elif defined CONFIG_CODEC_CHIP_IS_MICROSEMI
    .codec_init = msc_dac_init,
    .codec_uninit = msc_dac_uninit,
    .codec_sart = msc_dac_start,
    .codec_stop = msc_dac_stop,
    .codec_config_fmt = msc_dac_config_fmt,
    .codec_set_bit = msc_dac_set_bits,
    .codec_set_adc_input = msc_adc_input,
    .codec_set_vol = msc_dac_set_volume,
    .codec_get_vol = msc_dac_get_volume,
    .codec_set_mute = msc_dac_set_mute,
    .codec_get_mute = msc_dac_get_mute,
#elif defined CONFIG_CODEC_CHIP_IS_ES8311
    .codec_init = Es8311Init,
    .codec_uninit = Es8311Uninit,
    .codec_sart = Es8311Start,
    .codec_stop = Es8311Stop,
    .codec_config_fmt = Es8311ConfigFmt,
    .codec_set_bit = Es8311SetBitsPerSample,
    .codec_set_adc_input = Es8311ConfigAdcInput,
    .codec_set_vol = Es8311SetVoiceVolume,
    .codec_get_vol = Es8311GetVoiceVolume,
    .codec_set_mute = Es8311SetVoiceMute,
    .codec_get_mute = Es8311GetVoiceMute,
#endif
};

int MediaHalInit(void *config)
{
    int ret  = 0;
    // Set I2S pins
    if (I2S_NUM > 1 || I2S_NUM < 0) {
        ESP_LOGE(HAL_TAG, "Must set I2S_NUM as 0 or 1");
        return -1;
    }
    ret = i2s_driver_install(I2S_NUM, &i2s_config, 0, NULL);
    if (ret < 0) {
        ESP_LOGE(HAL_TAG, "I2S_NUM_0 install failed");
        return -1;
    }
#if I2S1_ENABLE
    ret = i2s_driver_install(I2S_NUM_1, &i2s_config, 0, NULL);
    if (ret < 0) {
        ESP_LOGE(HAL_TAG, "I2S_NUM_1 install failed");
        return -1;
    }
#endif

#ifdef CONFIG_USE_ES7243
    i2s_config.sample_rate = 16000;
    i2s_config.use_apll = 0;
    i2s_config.channel_format = I2S_CHANNEL_FMT_RIGHT_LEFT;
    ret = i2s_driver_install(I2S_NUM_1, &i2s_config, 0, NULL);
    if (ret < 0) {
        ESP_LOGE(HAL_TAG, "I2S_NUM_1 install failed");
        return -1;
    }
    ret |= i2s_set_pin(I2S_NUM_1, &i2s1_pin);
#elif defined CONFIG_USE_ES7210
    i2s_config.sample_rate = 16000;
    i2s_config.bits_per_sample = 32; // for ES7210
    i2s_config.use_apll = 0;
    i2s_config.channel_format = I2S_CHANNEL_FMT_RIGHT_LEFT;
    ret = i2s_driver_install(I2S_NUM_1, &i2s_config, 0, NULL);
    if (ret < 0) {
        ESP_LOGE(HAL_TAG, "I2S_NUM_1 install failed");
        return -1;
    }
    ret |= i2s_set_pin(I2S_NUM_1, &i2s1_pin);

    Es7210Config Es7210Conf = AUDIO_CODEC_ES7210_DEFAULT();
    Es7210Init(&Es7210Conf);
#endif

#ifdef ENABLE_MCLK_GPIO0
    if (I2S_NUM == 0) {
        SET_PERI_REG_BITS(PIN_CTRL, CLK_OUT1, 0, CLK_OUT1_S);
    }
    PIN_FUNC_SELECT(PERIPHS_IO_MUX_GPIO0_U, FUNC_GPIO0_CLK_OUT1);
#endif

#if I2S_DAC_EN == 1
    //init DAC pad
    i2s_set_dac_mode(I2S_DAC_CHANNEL_BOTH_EN);
#else
    ret |= i2s_set_pin(I2S_NUM, &i2s_pin);
    ret |= MediaHalConfig.codec_init(config);
    ret |= MediaHalConfig.codec_config_fmt(ES_MODULE_ADC_DAC, ES_I2S_NORMAL);
    ret |= MediaHalConfig.codec_set_bit(ES_MODULE_ADC_DAC, BIT_LENGTH_16BITS);
    ret |= MediaHalConfig.codec_set_adc_input(ADC_INPUT_LINPUT2_RINPUT2);
    ret |= MediaHalConfig.codec_sart(ES_MODULE_ADC_DAC);
#ifndef CONFIG_CODEC_CHIP_IS_MICROSEMI
    if (I2S_NUM == 0) {
        SET_PERI_REG_BITS(PIN_CTRL, CLK_OUT1, 0, CLK_OUT1_S);
    }
    PIN_FUNC_SELECT(PERIPHS_IO_MUX_GPIO0_U, FUNC_GPIO0_CLK_OUT1);
#endif // CONFIG_CODEC_CHIP_IS_MICROSEMI
#endif // I2S_DAC_EN

    MediaHalConfig._currentMode = CODEC_MODE_UNKNOWN;
    if (MediaHalConfig._halLock) {
        mutex_destroy(MediaHalConfig._halLock);
    }
    MediaHalConfig._halLock = mutex_init();
    gpio_config_t  io_conf;
    memset(&io_conf, 0, sizeof(io_conf));
    io_conf.intr_type = GPIO_PIN_INTR_DISABLE;
    io_conf.mode = GPIO_MODE_OUTPUT;
    io_conf.pin_bit_mask = ((1ULL<<GPIO_PA_EN));
    io_conf.pull_down_en = 0;
    io_conf.pull_up_en = 0;
    gpio_config(&io_conf);
    gpio_set_level(GPIO_PA_EN, 1);

    ret |= MediaHalConfig.codec_set_vol(I2S_OUT_VOL_DEFAULT);
    ESP_LOGI(HAL_TAG, "I2S_OUT_VOL_DEFAULT[%d]", I2S_OUT_VOL_DEFAULT);
    MediaHalConfig.sMediaHalState = MEDIA_HAL_STATE_INIT;
    return ret;
}

int MediaHalUninit(void)
{
    mutex_destroy(MediaHalConfig._halLock);
#if I2S_DAC_EN == 0
    MediaHalConfig.codec_uninit();
#endif
    MediaHalConfig._halLock = NULL;
    MUSIC_BITS = 0;
    MediaHalConfig._currentMode = CODEC_MODE_UNKNOWN;
    MediaHalConfig.sMediaHalState = MEDIA_HAL_STATE_UNKNOWN;
    return 0;
}

int MediaHalStart(CodecMode mode)
{
    int ret = 0;
#if I2S_DAC_EN == 0
    int esMode = 0;
    mutex_lock(MediaHalConfig._halLock);
    switch (mode) {
        case CODEC_MODE_ENCODE:
            esMode  = ES_MODULE_ADC;
            break;
        case CODEC_MODE_LINE_IN:
            esMode  = ES_MODULE_LINE;
            break;
        case CODEC_MODE_DECODE:
            esMode  = ES_MODULE_DAC;
            break;
        case CODEC_MODE_DECODE_ENCODE:
            esMode  = ES_MODULE_ADC_DAC;
            break;
        default:
            esMode = ES_MODULE_DAC;
            ESP_LOGW(HAL_TAG, "Codec mode not support, default is decode mode");
            break;
    }
    ESP_LOGI(HAL_TAG, "Codec mode is %d", esMode);
    int inputConfig = 0;

#ifdef CONFIG_CODECCHIP_IS_ES8388
    if (esMode == ES_MODULE_LINE) {
        inputConfig = ADC_INPUT_LINPUT2_RINPUT2;
    } else {
#if DIFFERENTIAL_MIC
        inputConfig = ADC_INPUT_DIFFERENCE;
#else
        inputConfig = ADC_INPUT_LINPUT1_RINPUT1;
#endif
    }
#elif defined CONFIG_CODECCHIP_IS_ES8374
#if DIFFERENTIAL_MIC
    inputConfig = ADC_INPUT_DIFFERENCE;
#else
    inputConfig = ADC_INPUT_MIC1;
#endif
#elif defined CONFIG_CODECCHIP_IS_ES7149
    if (esMode == ES_MODULE_LINE) {
        inputConfig = ADC_INPUT_LINPUT2_RINPUT2;
    } else {
#if DIFFERENTIAL_MIC
        inputConfig = ADC_INPUT_DIFFERENCE;
#else
        inputConfig = ADC_INPUT_LINPUT1_RINPUT1;
#endif
    }
#endif

    ret |= MediaHalConfig.codec_set_adc_input(inputConfig);
    ret |= MediaHalConfig.codec_sart(esMode);

    ESP_LOGI(HAL_TAG, "Codec inputConfig is %2X", inputConfig);

    MediaHalConfig._currentMode = mode;
    mutex_unlock(MediaHalConfig._halLock);
#endif /* I2S_DAC_EN */
    return ret;
}

int MediaHalStop(CodecMode mode)
{
    int ret = 0;
#if I2S_DAC_EN == 0
    int esMode = 0;
    mutex_lock(MediaHalConfig._halLock);
    switch (mode) {
        case CODEC_MODE_ENCODE:
            esMode  = ES_MODULE_ADC;
            break;
        case CODEC_MODE_LINE_IN:
            esMode  = ES_MODULE_LINE;
            break;
        case CODEC_MODE_DECODE:
            esMode  = ES_MODULE_DAC;
            break;
        default:
            esMode = ES_MODULE_DAC;
            ESP_LOGI(HAL_TAG, "Codec mode not support");
            break;
    }

    ret = MediaHalConfig.codec_stop(esMode);
    MediaHalConfig._currentMode = CODEC_MODE_UNKNOWN;
    mutex_unlock(MediaHalConfig._halLock);
#endif
    return ret;
}

int MediaHalGetCurrentMode(CodecMode *mode)
{
    MEDIA_HAL_CHECK_NULL(mode, "Get current mode para is null", -1);
    *mode = MediaHalConfig._currentMode;
    return 0;
}

int MediaHalSetVolume(int volume)
{
    int ret = 0;
    mutex_lock(MediaHalConfig._halLock);
#if I2S_DAC_EN == 0
    int mute;
    MediaHalConfig.codec_get_mute(&mute);
    if (volume < 3 ) {
        if (0 == mute) {
            MediaHalConfig.codec_set_mute(CODEC_MUTE_ENABLE);
        }
    } else {
        if ((1 == mute)) {
            MediaHalConfig.codec_set_mute(CODEC_MUTE_DISABLE);
        }
    }
    ret = MediaHalConfig.codec_set_vol(volume);

    mutex_unlock(MediaHalConfig._halLock);
#endif
    return ret;
}

int MediaHalGetVolume(int *volume)
{
    int ret = 0;
    MEDIA_HAL_CHECK_NULL(volume, "Get volume para is null", -1);
#if I2S_DAC_EN == 0
    mutex_lock(MediaHalConfig._halLock);
    ret = MediaHalConfig.codec_get_vol(volume);
    mutex_unlock(MediaHalConfig._halLock);
#endif
    return ret;
}

void MediaHalSetVolumeAmplify(float scale)
{
    mutex_lock(MediaHalConfig._halLock);
    AMPLIFIER = scale * (1 << 8);
    mutex_unlock(MediaHalConfig._halLock);
}

int MediaHalGetVolumeAmplify()
{
    return AMPLIFIER;
}

int MediaHalGetAmplifyType()
{
    return MediaHalConfig.amplifier_type;
}

int MediaHalSetMute(CodecMute mute)
{
    int ret = 0;
#if I2S_DAC_EN == 0
    mutex_lock(MediaHalConfig._halLock);
    ret = MediaHalConfig.codec_set_mute(mute);
    mutex_unlock(MediaHalConfig._halLock);
#endif
    return ret;
}

int MediaHalGetMute(void)
{
    int mute = 0;
#if I2S_DAC_EN == 0
    mutex_lock(MediaHalConfig._halLock);
    MediaHalConfig.codec_get_mute(&mute);
    mutex_unlock(MediaHalConfig._halLock);
#endif
    return mute;
}

int MediaHalSetBits(int bitPerSample)
{
    int ret = 0;
    if (bitPerSample <= BIT_LENGTH_MIN || bitPerSample >= BIT_LENGTH_MAX) {
        ESP_LOGE(HAL_TAG, "bitPerSample: wrong param");
        return -1;
    }
#if I2S_DAC_EN == 0
    mutex_lock(MediaHalConfig._halLock);
    MediaHalConfig.codec_set_bit(ES_MODULE_ADC_DAC, (BitsLength)bitPerSample);
    mutex_unlock(MediaHalConfig._halLock);
#endif
    return ret;
}

int MediaHalSetClk(int i2s_num, uint32_t rate, uint8_t bits, uint32_t ch)
{
    int ret;
    if (bits != 16 && bits != 32) {
        ESP_LOGE(HAL_TAG, "bit should be 16 or 32, Bit:%d", bits);
        return -1;
    }
    if (ch != 1 && ch != 2) {
        ESP_LOGE(HAL_TAG, "channel should be 1 or 2 %d", ch);
        return -1;
    }
    if (bits > SUPPOERTED_BITS) {
        ESP_LOGE(HAL_TAG, "Bits:%d, bits must be smaller than %d", bits, SUPPOERTED_BITS);
        return -1;
    }

    MUSIC_BITS = bits;
    ret = i2s_set_clk((i2s_port_t)i2s_num, rate, SUPPOERTED_BITS, ch);

    return ret;
}

int MediaHalGetI2sConfig(int i2sNum, void *info)
{
    if (info) {
        memcpy(info, &i2s_config, sizeof(i2s_config_t));
    }
#if defined CONFIG_ESP_LYRATD_FT_V1_0_BOARD || defined CONFIG_ESP_LYRATD_FT_DOSS_V1_0_BOARD
    ((i2s_config_t *)info)->sample_rate = 16000;
#endif
    return 0;
}

int MediaHalGetI2sNum(void)
{
    return I2S_NUM;
}

int MediaHalGetI2sBits(void)
{
    return SUPPOERTED_BITS;
}

int MediaHalGetSrcBits(void)
{
    return MUSIC_BITS;
}

int MediaHalGetI2sDacMode(void)
{
    return I2S_DAC_EN;
}

int MediaHalGetI2sAdcMode(void)
{
    return I2S_ADC_EN;
}

int MediaHalPaPwr(int en)
{
    if (en) {
        gpio_set_level(GPIO_PA_EN, 1);
    } else {
        gpio_set_level(GPIO_PA_EN, 0);
    }
    return 0;
}

int MediaHalGetState(MediaHalState *state)
{
    if (state) {
        *state = MediaHalConfig.sMediaHalState;
        return 0;
    }
    return -1;
}

void codec_init(void)
{
    int ret = 0;
#if (defined CONFIG_CODEC_CHIP_IS_ES8388)
    Es8388Config  Es8388Conf =  AUDIO_CODEC_ES8388_DEFAULT();
    ret = MediaHalInit(&Es8388Conf);
    if (ret) {
        ESP_LOGE(HAL_TAG, "MediaHal init failed, line:%d", __LINE__);
    }
    ESP_LOGI(HAL_TAG, "CONFIG_CODEC_CHIP_IS_ES8388");
#elif (defined CONFIG_CODEC_CHIP_IS_ES8374)
    Es8374Config  Es8374Conf =  AUDIO_CODEC_ES8374_DEFAULT();
    ret = MediaHalInit(&Es8374Conf);
    if (ret) {
        ESP_LOGI(HAL_TAG, "MediaHal init failed, line:%d", __LINE__);
    }
    ESP_LOGI(HAL_TAG, "CONFIG_CODEC_CHIP_IS_ES8374");

#elif (defined CONFIG_CODEC_CHIP_IS_ES8311)
    Es8311Config  es8311Cfg =  AUDIO_CODEC_ES8311_DEFAULT();
    ret = MediaHalInit(&es8311Cfg);
    if (ret) {
        ESP_LOGI(HAL_TAG, "MediaHal init failed, line:%d", __LINE__);
    }
    ESP_LOGI(HAL_TAG, "CONFIG_CODEC_CHIP_IS_ES8311");

#endif
}