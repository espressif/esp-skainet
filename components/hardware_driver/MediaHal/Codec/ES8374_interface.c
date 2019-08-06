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

#include "ES8374_interface.h"
#include "esp_system.h"
#include "esp_log.h"

#define ES8374_TAG "8374"

#define ES_ASSERT(a, format, b, ...) \
    if ((a) != 0) { \
        ESP_LOGE(ES8374_TAG, format, ##__VA_ARGS__); \
        return b;\
    }

#define LOG_8374(fmt, ...)   ESP_LOGW(ES8374_TAG, fmt, ##__VA_ARGS__)

/**
 * @brief Write ES8374 register
 *
 * @param slaveAdd  : slave address
 * @param regAdd    : register address
 * @param data      : data to write
 *
 * @return
 *     - (-1)  Error
 *     - (0)   Success
 */
int Es8374WriteReg(uint8_t slaveAdd, uint8_t regAdd, uint8_t data)
{
    int res = 0;
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    res |= i2c_master_start(cmd);
    res |= i2c_master_write_byte(cmd, slaveAdd, 1 /*ACK_CHECK_EN*/);
    res |= i2c_master_write_byte(cmd, regAdd, 1 /*ACK_CHECK_EN*/);
    res |= i2c_master_write_byte(cmd, data, 1 /*ACK_CHECK_EN*/);
    res |= i2c_master_stop(cmd);
    res |= i2c_master_cmd_begin(0, cmd, 1000 / portTICK_RATE_MS);
    i2c_cmd_link_delete(cmd);
    ES_ASSERT(res, "ESCodecWriteReg error", -1);
    return res;
}

/**
 * @brief Read ES8374 register
 *
 * @param slaveAdd : slave address
 * @param regAdd   : register address
 * @param pData    : data to read
 *
 * @return
 *     - (-1)  Error
 *     - (0)   Success
 */
int Es8374ReadReg(uint8_t slaveAdd, uint8_t regAdd, uint8_t *pData)
{
    uint8_t data;
    int res = 0;
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();

    res |= i2c_master_start(cmd);
    res |= i2c_master_write_byte(cmd, slaveAdd, 1 /*ACK_CHECK_EN*/);
    res |= i2c_master_write_byte(cmd, regAdd, 1 /*ACK_CHECK_EN*/);
    res |= i2c_master_stop(cmd);
    res |= i2c_master_cmd_begin(0, cmd, 1000 / portTICK_RATE_MS);
    i2c_cmd_link_delete(cmd);

    cmd = i2c_cmd_link_create();
    res |= i2c_master_start(cmd);
    res |= i2c_master_write_byte(cmd, slaveAdd | 0x01, 1 /*ACK_CHECK_EN*/);
    res |= i2c_master_read_byte(cmd, &data, 0x01/*NACK_VAL*/);
    res |= i2c_master_stop(cmd);
    res |= i2c_master_cmd_begin(0, cmd, 1000 / portTICK_RATE_MS);
    i2c_cmd_link_delete(cmd);

    ES_ASSERT(res, "Es8374ReadReg error", -1);
    *pData = data;
    return res;
}

/**
 * @brief Inite i2s master mode
 *
 * @return
 *     - (-1)  Error
 *     - (0)  Success
 */
int Es8374I2cInit(i2c_config_t *conf, int i2cMasterPort)
{
    int res;

    res = i2c_param_config(i2cMasterPort, conf);
    res |= i2c_driver_install(i2cMasterPort, conf->mode, 0, 0, 0);
    ES_ASSERT(res, "I2cInit error", -1);
    return res;
}

int ES8374WriteReg(uint8_t regAdd, uint8_t data)
{
    return Es8374WriteReg(ES8374_ADDR, regAdd, data);
}

int ES8374ReadReg(uint8_t regAdd, uint8_t *regv)
{
    uint8_t regdata = 0xFF;
    uint8_t res =0;

    if(Es8374ReadReg(ES8374_ADDR, regAdd, &regdata) == 0) {
        *regv = regdata;
        return res;
    }
    else
    {
        LOG_8374("Read Audio Codec Register Failed!\n");
        res = -1;
        return res;
    }
}

int Es8374SetVoiceMute(int mute)
{
    int res = 0;
    uint8_t reg = 0;

    res |= ES8374ReadReg(0x36, &reg);
    if(res == 0) {
        reg = reg & 0xdf;
        res |= ES8374WriteReg(0x36, reg | (((int)mute) << 5));
    }

    return res;
}

int Es8374GetVoiceMute(int *mute)
{
    int res = 0;
    uint8_t reg = 0;

    res |= ES8374ReadReg(0x36, &reg);
    if (res == ESP_OK) {
        reg = reg & 0x40;
    }
    *mute = reg;

    return res;
}

int Es8374SetBitsPerSample(ESCodecModule mode, BitsLength bitPerSample)
{
    int res = 0;
    uint8_t reg = 0;
    int bits = (int)bitPerSample & 0x0f;

    if (mode == ES_MODULE_ADC || mode == ES_MODULE_ADC_DAC) {
        res |= ES8374ReadReg(0x10, &reg);
        if(res == 0) {
            reg = reg & 0xe3;
            res |=  ES8374WriteReg(0x10, reg | (bits << 2));
        }
    }
    if (mode == ES_MODULE_DAC || mode == ES_MODULE_ADC_DAC) {
        res |= ES8374ReadReg(0x11, &reg);
        if(res == 0) {
            reg = reg & 0xe3;
            res |= ES8374WriteReg(0x11, reg | (bits << 2));
        }
    }

    return res;
}

int Es8374ConfigFmt(ESCodecModule mode, ESCodecI2SFmt fmt)
{
    int res = 0;
    uint8_t reg = 0;
    int fmt_tmp,fmt_i2s;

    fmt_tmp = ((fmt & 0xf0) >> 4);
    fmt_i2s =  fmt & 0x0f;
    if (mode == ES_MODULE_ADC || mode == ES_MODULE_ADC_DAC) {
        res |= ES8374ReadReg(0x10, &reg);
        if(res == 0) {
            reg = reg & 0xfc;
            res |= ES8374WriteReg(0x10, reg | fmt_i2s);
            res |= Es8374SetBitsPerSample(mode, fmt_tmp);
        }
    }
    if (mode == ES_MODULE_DAC || mode == ES_MODULE_ADC_DAC) {
        res |= ES8374ReadReg(0x11, &reg);
        if(res == 0) {
            reg = reg & 0xfc;
            res |= ES8374WriteReg(0x11, reg | (fmt_i2s));
            res |= Es8374SetBitsPerSample(mode, fmt_tmp);
        }
    }

    return res;
}

int Es8374Start(ESCodecModule mode)
{
    int res = 0;
    uint8_t reg = 0;

    if (mode == ES_MODULE_LINE) {
        res |= ES8374ReadReg(0x1a, &reg);       //set monomixer
        reg |= 0x60;
        reg |= 0x20;
        reg &= 0xf7;
        res |= ES8374WriteReg( 0x1a, reg);
        res |= ES8374ReadReg(0x1c, &reg);        // set spk mixer
        reg |= 0x40;
        res |= ES8374WriteReg( 0x1c, reg);
        res |= ES8374WriteReg(0x1D,0x02);       // spk set
        res |= ES8374WriteReg(0x1F,0x00);       // spk set
        res |= ES8374WriteReg(0x1E,0xA0);       // spk on
    }
    if (mode == ES_MODULE_ADC || mode == ES_MODULE_ADC_DAC || mode == ES_MODULE_LINE) {
        res |= ES8374ReadReg(0x21, &reg);       //power up adc and input
        reg &= 0x3f;
        res |= ES8374WriteReg(0x21,reg);
        res |= ES8374ReadReg(0x10, &reg);       //power up adc and input
        reg &= 0x3f;
        res |= ES8374WriteReg(0x10,reg);
    }

    if (mode == ES_MODULE_DAC || mode == ES_MODULE_ADC_DAC || mode == ES_MODULE_LINE) {
        res |= ES8374ReadReg(0x1a, &reg);       //disable lout
        reg |= 0x08;
        res |= ES8374WriteReg( 0x1a, reg);
        reg &= 0xdf;
        res |= ES8374WriteReg( 0x1a, reg);
        res |= ES8374WriteReg(0x1D,0x12);       // mute speaker
        res |= ES8374WriteReg(0x1E,0x20);       // disable class d
        res |= ES8374ReadReg(0x15, &reg);        //power up dac
        reg &= 0xdf;
        res |= ES8374WriteReg(0x15,reg);
        res |= ES8374ReadReg(0x1a, &reg);        //disable lout
        reg |= 0x20;
        res |= ES8374WriteReg( 0x1a, reg);
        reg &= 0xf7;
        res |= ES8374WriteReg( 0x1a, reg);
        res |= ES8374WriteReg(0x1D,0x02);       // mute speaker
        res |= ES8374WriteReg(0x1E,0xa0);       // disable class d

        res |= Es8374SetVoiceMute(false);
    }

    return res;
}


int Es8374Stop(ESCodecModule mode)
{
    int res = 0;
    uint8_t reg = 0;

    if (mode == ES_MODULE_LINE) {
        res |= ES8374ReadReg(0x1a, &reg);       //disable lout
        reg |= 0x08;
        res |= ES8374WriteReg( 0x1a, reg);
        reg &= 0x9f;
        res |= ES8374WriteReg( 0x1a, reg);
        res |= ES8374WriteReg(0x1D,0x12);       // mute speaker
        res |= ES8374WriteReg(0x1E,0x20);       // disable class d
        res |= ES8374ReadReg(0x1c, &reg);        // disable spkmixer
        reg &= 0xbf;
        res |= ES8374WriteReg( 0x1c, reg);
        res |= ES8374WriteReg(0x1F,0x00);       // spk set
    }
    if (mode == ES_MODULE_DAC || mode == ES_MODULE_ADC_DAC) {
        res |= Es8374SetVoiceMute(true);

        res |= ES8374ReadReg(0x1a, &reg);        //disable lout
        reg |= 0x08;
        res |= ES8374WriteReg( 0x1a, reg);
        reg &= 0xdf;
        res |= ES8374WriteReg( 0x1a, reg);
        res |= ES8374WriteReg(0x1D,0x12);       // mute speaker
        res |= ES8374WriteReg(0x1E,0x20);       // disable class d
        res |= ES8374ReadReg(0x15, &reg);        //power up dac
        reg |= 0x20;
        res |= ES8374WriteReg(0x15,reg);
    }
    if (mode == ES_MODULE_ADC || mode == ES_MODULE_ADC_DAC) {

        res |= ES8374ReadReg(0x10, &reg);       //power up adc and input
        reg |= 0xc0;
        res |= ES8374WriteReg(0x10,reg);
        res |= ES8374ReadReg(0x21, &reg);       //power up adc and input
        reg |= 0xc0;
        res |= ES8374WriteReg(0x21,reg);
    }

    return res;
}

int Es8374I2sConfigClock(ESCodecI2sClock cfg)
{

    int res = 0;
    uint8_t reg = 0;

    res |= ES8374ReadReg(0x0f, &reg);       //power up adc and input
    reg &= 0xe0;
    int divratio = 0;
    switch(cfg.sclkDiv) {
        case MclkDiv_1:
            divratio = 1;
            break;
        case MclkDiv_2: // = 2,
            divratio = 2;
            break;
        case MclkDiv_3: // = 3,
            divratio = 3;
            break;
        case MclkDiv_4: // = 4,
            divratio = 4;
            break;
        case MclkDiv_5: // = 20,
            divratio = 5;
            break;
        case MclkDiv_6: // = 5,
            divratio = 6;
            break;
        case MclkDiv_7: //  = 29,
            divratio = 7;
            break;
        case MclkDiv_8: // = 6,
            divratio = 8;
            break;
        case MclkDiv_9: // = 7,
            divratio = 9;
            break;
        case MclkDiv_10: // = 21,
            divratio = 10;
            break;
        case MclkDiv_11: // = 8,
            divratio = 11;
            break;
        case MclkDiv_12: // = 9,
            divratio = 12;
            break;
        case MclkDiv_13: // = 30,
            divratio = 13;
            break;
        case MclkDiv_14: // = 31
            divratio = 14;
            break;
        case MclkDiv_15: // = 22,
            divratio = 15;
            break;
        case MclkDiv_16: // = 10,
            divratio = 16;
            break;
        case MclkDiv_17: // = 23,
            divratio = 17;
            break;
        case MclkDiv_18: // = 11,
            divratio = 18;
            break;
        case MclkDiv_20: // = 24,
            divratio = 19;
            break;
        case MclkDiv_22: // = 12,
            divratio = 20;
            break;
        case MclkDiv_24: // = 13,
            divratio = 21;
            break;
        case MclkDiv_25: // = 25,
            divratio = 22;
            break;
        case MclkDiv_30: // = 26,
            divratio = 23;
            break;
        case MclkDiv_32: // = 27,
            divratio = 24;
            break;
        case MclkDiv_33: // = 14,
            divratio = 25;
            break;
        case MclkDiv_34: // = 28,
            divratio = 26;
            break;
        case MclkDiv_36: // = 15,
            divratio = 27;
            break;
        case MclkDiv_44: // = 16,
            divratio = 28;
            break;
        case MclkDiv_48: // = 17,
            divratio = 29;
            break;
        case MclkDiv_66: // = 18,
            divratio = 30;
            break;
        case MclkDiv_72: // = 19,
            divratio = 31;
            break;
        default:
            break;
    }
    reg |= divratio;
    res |= ES8374WriteReg(0x0f,reg);

    int dacratio_l = 0;
    int dacratio_h = 0;

    switch(cfg.lclkDiv)
    {
        case LclkDiv_128:
            dacratio_l = 128 % 256;
            dacratio_h = 128 / 256;
            break;
        case LclkDiv_192:
            dacratio_l = 192 % 256;
            dacratio_h = 192 / 256;
            break;
        case LclkDiv_256:
            dacratio_l = 256 % 256;
            dacratio_h = 256 / 256;
            break;
        case LclkDiv_384:
            dacratio_l = 384 % 256;
            dacratio_h = 384 / 256;
            break;
        case LclkDiv_512:
            dacratio_l = 512 % 256;
            dacratio_h = 512 / 256;
            break;
        case LclkDiv_576:
            dacratio_l = 576 % 256;
            dacratio_h = 576 / 256;
            break;
        case LclkDiv_768:
            dacratio_l = 768 % 256;
            dacratio_h = 768 / 256;
            break;
        case LclkDiv_1024:
            dacratio_l = 1024 % 256;
            dacratio_h = 1024 / 256;
            break;
        case LclkDiv_1152:
            dacratio_l = 1152 % 256;
            dacratio_h = 1152 / 256;
            break;
        case LclkDiv_1408:
            dacratio_l = 1408 % 256;
            dacratio_h = 1408 / 256;
            break;
        case LclkDiv_1536:
            dacratio_l = 1536 % 256;
            dacratio_h = 1536 / 256;
            break;
        case LclkDiv_2112:
            dacratio_l = 2112 % 256;
            dacratio_h = 2112 / 256;
            break;
        case LclkDiv_2304:
            dacratio_l = 2304 % 256;
            dacratio_h = 2304 / 256;
            break;
        case LclkDiv_125:
            dacratio_l = 125 % 256;
            dacratio_h = 125 / 256;
            break;
        case LclkDiv_136:
            dacratio_l = 136 % 256;
            dacratio_h = 136 / 256;
            break;
        case LclkDiv_250:
            dacratio_l = 250 % 256;
            dacratio_h = 250 / 256;
            break;
        case LclkDiv_272:
            dacratio_l = 272 % 256;
            dacratio_h = 272 / 256;
            break;
        case LclkDiv_375:
            dacratio_l = 375 % 256;
            dacratio_h = 375 / 256;
            break;
        case LclkDiv_500:
            dacratio_l = 500 % 256;
            dacratio_h = 500 / 256;
            break;
        case LclkDiv_544:
            dacratio_l = 544 % 256;
            dacratio_h = 544 / 256;
            break;
        case LclkDiv_750:
            dacratio_l = 750 % 256;
            dacratio_h = 750 / 256;
            break;
        case LclkDiv_1000:
            dacratio_l = 1000 % 256;
            dacratio_h = 1000 / 256;
            break;
        case LclkDiv_1088:
            dacratio_l = 1088 % 256;
            dacratio_h = 1088 / 256;
            break;
        case LclkDiv_1496:
            dacratio_l = 1496 % 256;
            dacratio_h = 1496 / 256;
            break;
        case LclkDiv_1500:
            dacratio_l = 1500 % 256;
            dacratio_h = 1500 / 256;
            break;
        default:
            break;
    }
    res |= ES8374WriteReg( 0x06, dacratio_h);  //ADCFsMode,singel SPEED,RATIO=256
    res |= ES8374WriteReg( 0x07, dacratio_l);  //ADCFsMode,singel SPEED,RATIO=256

    return res;
}

int Es8374ConfigDacOutput(DacOutput dacoutput)
{
    int res = 0;
    uint8_t reg = 0;

    reg = 0x1d;

    res = ES8374WriteReg(reg, 0x02);
    res |= ES8374ReadReg(0x1c, &reg);        // set spk mixer
    reg |= 0x80;
    res |= ES8374WriteReg( 0x1c, reg);
    res |= ES8374WriteReg(0x1D,0x02); // spk set
    res |= ES8374WriteReg(0x1F,0x00); // spk set
    res |= ES8374WriteReg(0x1E,0xA0); // spk on

    return res;
}

int Es8374ConfigAdcInput(AdcInput adcinput)
{
    int res = 0;
    uint8_t reg = 0;

    res |= ES8374ReadReg(0x21, &reg);
    if(res == 0) {
        reg = (reg & 0xcf) | 0x14;
        res |= ES8374WriteReg( 0x21, reg);
    }

    return res;
}

int Es8374SetMicGain(MicGain gain)
{
    int res = 0;

    if(gain > MIC_GAIN_MIN && gain < MIC_GAIN_24DB) {
        int gain_n = 0;
        gain_n = (int)gain / 3;
        res = ES8374WriteReg(0x22, gain_n | (gain_n << 4)); //MIC PGA
    }
    else
    {
        res = -1;
        LOG_8374("invalid microphone gain!\n");
    }

    return res;
}

int Es8374SetVoiceVolume(int volume)
{
    int res = 0;

    if (volume < 0) {
        volume = 192;
    } else if(volume > 96) {
        volume = 0;
    } else {
        volume = 192 - volume*2;
    }

    res = ES8374WriteReg(0x38, volume);

    return res;
}

int Es8374GetVoiceVolume(int *volume)
{
    int res = 0;
    uint8_t reg = 0;

    res = ES8374ReadReg(0x38, &reg);

    if (res == ESP_FAIL) {
        *volume = 0;
    } else {
        *volume = (192 - reg)/2;
        if(*volume > 96)
            *volume = 100;
    }

    return res;
}

int Es8374SetAdcDacVolume(ESCodecModule mode, int volume, int dot)
{
    int res = 0;

    if ( volume < -96 || volume > 0 ) {
        LOG_8374("Warning: volume < -96! or > 0!\n");
        if (volume < -96)
            volume = -96;
        else
            volume = 0;
    }
    dot = (dot >= 5 ? 1 : 0);
    volume = (-volume << 1) + dot;
    if (mode == ES_MODULE_ADC || mode == ES_MODULE_ADC_DAC) {
        res |= ES8374WriteReg(0x25, volume);
    }
    if (mode == ES_MODULE_DAC || mode == ES_MODULE_ADC_DAC) {
        res |= ES8374WriteReg(0x38, volume);
    }

    return res;
}

int Es8374D2sePga(D2SEPGA gain)
{
    int res = 0;
    uint8_t reg = 0;

    if(gain > D2SE_PGA_GAIN_MIN && gain < D2SE_PGA_GAIN_MAX) {
        res = ES8374ReadReg(0x21, &reg);
        reg &= 0xfb;
        reg |= gain << 2;
        res = ES8374WriteReg(0x21, reg); //MIC PGA
    }
    else
    {
        res = 0xff;
        LOG_8374("invalid microphone gain!\n");
    }

    return res;
}

int Es8374InitReg(ESCodecMode ms_mode, ESCodecI2SFmt fmt, ESCodecI2sClock cfg, DacOutput out_channel, AdcInput in_channel)
{
    int res = 0;
    uint8_t reg;

    res |= ES8374WriteReg(0x00,0x3F); //IC Rst start
    res |= ES8374WriteReg(0x00,0x03); //IC Rst stop
    res |= ES8374WriteReg(0x01,0x7F); //IC clk on

    res |= ES8374ReadReg(0x0F, &reg);
    reg &= 0x7f;
    reg |=  (ms_mode << 7);
    res |= ES8374WriteReg( 0x0f, reg); //CODEC IN I2S SLAVE MODE

    res |= ES8374WriteReg(0x6F,0xA0); //pll set:mode enable
    res |= ES8374WriteReg(0x72,0x41); //pll set:mode set
    res |= ES8374WriteReg(0x09,0x01); //pll set:reset on ,set start
    res |= ES8374WriteReg(0x0C,0x22); //pll set:k
    res |= ES8374WriteReg(0x0D,0x2E); //pll set:k
    res |= ES8374WriteReg(0x0E,0xC6); //pll set:k
    res |= ES8374WriteReg(0x0A,0x3A); //pll set:
    res |= ES8374WriteReg(0x0B,0x07); //pll set:n
    res |= ES8374WriteReg(0x09,0x41); //pll set:reset off ,set stop

    res |= Es8374I2sConfigClock(cfg);

    res |= ES8374WriteReg(0x24,0x08); //adc set
    res |= ES8374WriteReg(0x36,0x00); //dac set
    res |= ES8374WriteReg(0x12,0x30); //timming set
    res |= ES8374WriteReg(0x13,0x20); //timming set

    res |= Es8374ConfigFmt(ES_MODULE_ADC, fmt);
    res |= Es8374ConfigFmt(ES_MODULE_DAC, fmt);

    res |= ES8374WriteReg(0x21,0x50); //adc set: SEL LIN1 CH+PGAGAIN=0DB
    res |= ES8374WriteReg(0x22,0xFF); //adc set: PGA GAIN=0DB
    res |= ES8374WriteReg(0x21,0x14); //adc set: SEL LIN1 CH+PGAGAIN=18DB
    res |= ES8374WriteReg(0x22,0x55); //pga = +15db
    res |= ES8374WriteReg(0x08,0x21); //set class d divider = 33, to avoid the high frequency tone on laudspeaker
    res |= ES8374WriteReg(0x00,0x80); // IC START

    res |= Es8374SetAdcDacVolume(ES_MODULE_ADC, 0, 0);      // 0db
    res |= Es8374SetAdcDacVolume(ES_MODULE_DAC, 0, 0);      // 0db

    res |= ES8374WriteReg(0x14,0x8A); // IC START
    res |= ES8374WriteReg(0x15,0x40); // IC START
    res |= ES8374WriteReg(0x1A,0xA0); // monoout set
    res |= ES8374WriteReg(0x1B,0x19); // monoout set
    res |= ES8374WriteReg(0x1C,0x90); // spk set
    res |= ES8374WriteReg(0x1D,0x01); // spk set
    res |= ES8374WriteReg(0x1F,0x00); // spk set
    res |= ES8374WriteReg(0x1E,0x20); // spk on
    res |= ES8374WriteReg(0x28,0x00); // alc set
    res |= ES8374WriteReg(0x25,0x00); // ADCVOLUME on
    res |= ES8374WriteReg(0x38,0x00); // DACVOLUME on
    res |= ES8374WriteReg(0x37,0x30); // dac set
    res |= ES8374WriteReg(0x6D,0x60); //SEL:GPIO1=DMIC CLK OUT+SEL:GPIO2=PLL CLK OUT
    res |= ES8374WriteReg(0x71,0x05); //for automute setting
    res |= ES8374WriteReg(0x73,0x70);

    res |= Es8374ConfigDacOutput(out_channel);  //0x3c Enable DAC and Enable Lout/Rout/1/2
    res |= Es8374ConfigAdcInput(in_channel);  //0x00 LINSEL & RINSEL, LIN1/RIN1 as ADC Input; DSSEL,use one DS Reg11; DSR, LINPUT1-RINPUT1
    res |= Es8374SetVoiceVolume(0);

    res |= ES8374WriteReg(0x37,0x00); // dac set

    return res;
}

int Es8374Init(Es8374Config *cfg)
{
    int res = 0;
    ESCodecI2sClock clkdiv;

    clkdiv.lclkDiv = LclkDiv_256;
    clkdiv.sclkDiv = MclkDiv_4;

    Es8374I2cInit(&cfg->i2c_cfg, cfg->i2c_port_num); // ESP32 in master mode

    res |= Es8374Stop(ES_MODULE_ADC_DAC);
    res |= Es8374InitReg(cfg->esMode, (BIT_LENGTH_16BITS << 4) | ES_I2S_NORMAL, clkdiv,
                        DAC_OUTPUT_SPK, ADC_INPUT_DIFFERENCE | ADC_INPUT_MIC1);
    res |= Es8374SetMicGain(MIC_GAIN_15DB);
    res |= Es8374D2sePga(D2SE_PGA_GAIN_EN);

    return res;
}

void Es8374Uninit()
{

    ES8374WriteReg(0x00,0x7F); // IC Rest and STOP

}

