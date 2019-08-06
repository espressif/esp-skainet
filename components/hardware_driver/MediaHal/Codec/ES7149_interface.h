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

#ifndef __Es7149_INTERFACE_H__
#define __Es7149_INTERFACE_H__

#include "ESCodec_common.h"
#include "esp_types.h"
#include "driver/i2c.h"

typedef struct {
    ESCodecMode esMode;
    DacOutput dacOutput;
    AdcInput adcInput;
} Es7149Config;

int Es7149Init(Es7149Config* cfg);
void Es7149Uninit();

int Es7149ConfigFmt(ESCodecModule mode, ESCodecI2SFmt fmt);

/**
 * @brief Es7149I2sConfigClock used for Es7149 MSATER mode
 */
int Es7149I2sConfigClock(ESCodecI2sClock cfg);
int Es7149SetBitsPerSample(ESCodecModule mode, BitsLength bitPerSample);

int Es7149Start(ESCodecModule mode);
int Es7149Stop(ESCodecModule mode);

int Es7149SetVoiceVolume(int volume);
int Es7149GetVoiceVolume(int* volume);
int Es7149SetVoiceMute(int enable);
int Es7149GetVoiceMute(int* mute);
int Es7149SetMicGain(MicGain gain);

int Es7149ConfigAdcInput(AdcInput input);
/**
 * @exemple Es7149ConfigDacOutput(DAC_OUTPUT_LOUT1 | DAC_OUTPUT_LOUT2 | DAC_OUTPUT_ROUT1 | DAC_OUTPUT_ROUT2);
 */
int Es7149ConfigDacOutput(DacOutput output);

void Es7149ReadAll();


#endif //__Es7149_INTERFACE_H__
