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

#ifndef __ES8374_INTERFACE_H__
#define __ES8374_INTERFACE_H__

#include "ESCodec_common.h"
#include "esp_types.h"
#include "driver/i2c.h"
#include "userconfig.h"

/* ES8374 address */
#define ES8374_ADDR 0x20  // 0x22:CE=1;0x20:CE=0

typedef struct {
    ESCodecMode esMode;
    i2c_port_t i2c_port_num;
    i2c_config_t i2c_cfg;
    DacOutput dacOutput;
    AdcInput adcInput;
} Es8374Config;

#define AUDIO_CODEC_ES8374_DEFAULT(){\
    .esMode = ES_MODE_SLAVE, \
    .i2c_port_num = I2C_NUM_0, \
    .i2c_cfg = {  \
        .mode = I2C_MODE_MASTER,  \
        .sda_io_num = IIC_DATA, \
        .scl_io_num = IIC_CLK, \
        .sda_pullup_en = GPIO_PULLUP_ENABLE, \
        .scl_pullup_en = GPIO_PULLUP_ENABLE, \
        .master.clk_speed = 100000 \
    }, \
    .adcInput = ADC_INPUT_MIC1, \
    .dacOutput = DAC_OUTPUT_SPK, \
};

int Es8374Init(Es8374Config *cfg);
void Es8374Uninit();
int Es8374Start(ESCodecModule mode);
int Es8374Stop(ESCodecModule mode);

int Es8374I2sConfigClock(ESCodecI2sClock cfg);
int Es8374SetBitsPerSample(ESCodecModule mode, BitsLength bitPerSample);
int Es8374ConfigFmt(ESCodecModule mode, ESCodecI2SFmt fmt);
int Es8374ConfigDacOutput(DacOutput dacoutput);
int Es8374ConfigAdcInput(AdcInput adcinput);
int Es8374SetMicGain(MicGain gain);
int Es8374SetVoiceMute(int mute);
int Es8374GetVoiceMute(int *mute);
int Es8374SetVoiceVolume(int volume);
int Es8374GetVoiceVolume(int *volume);

int ES8374WriteReg(uint8_t regAdd, uint8_t data);
int ES8374ReadReg(uint8_t regAdd, uint8_t *regv);

#endif  //__ES8374_INTERFACE_H__

