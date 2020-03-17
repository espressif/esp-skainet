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

#ifndef _ESP_AUDIO_BOARD_H_
#define _ESP_AUDIO_BOARD_H_

#include "sdkconfig.h"

#ifdef __cplusplus
extern "C" {
#endif

#ifdef CONFIG_ESP_LYRAT_V4_3_BOARD
#define CONFIG_CODEC_CHIP_IS_ES8388
#include "lyrat_v4_3_board.h"
#endif

#ifdef CONFIG_ESP_LYRAT_V4_2_BOARD
#include "lyrat_v4_2_board.h"
#endif

#ifdef CONFIG_ESP_LYRATD_MSC_V2_1_BOARD
#include "lyratd_msc_v2_1_board.h"
#endif

#ifdef CONFIG_ESP_LYRATD_MSC_V2_2_BOARD
#include "lyratd_msc_v2_2_board.h"
#endif

#ifdef CONFIG_ESP_LYRATD_MSC_BV1_0_BOARD
#include "lyratd_msc_bv1_0_board.h"
#endif

#ifdef CONFIG_ESP_LYRATD_FT_V1_0_BOARD
#include "lyratd_ft_v1_0_board.h"
#endif

#ifdef CONFIG_ESP_LYRATD_KN_V1_0_BOARD
#include "lyratd_kn_v1_0_board.h"
#endif

#ifdef CONFIG_ESP_LYRATD_FT_DOSS_V1_0_BOARD
#include "lyratd_ft_doss_v1_0_board.h"
#endif

#ifdef CONFIG_ESP_LYRAT_MINI_V1_0_BOARD
#include "lyrat_mini_v1_0_board.h"
#endif

#ifdef CONFIG_ESP_LYRAT_MINI_V1_1_BOARD
#define CONFIG_CODEC_CHIP_IS_ES8311
#define CONFIG_USE_ES7243
#include "lyrat_mini_v1_1_board.h"
#endif 

#ifdef CONFIG_ESP32_KORVO_V1_1_BOARD
#define CONFIG_CODEC_CHIP_IS_ES8311
#define CONFIG_USE_ES7210
#include "esp32_korvo_v1_1_board.h"
#endif 

#ifdef __cplusplus
}
#endif

#endif
