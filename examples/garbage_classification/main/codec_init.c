/* 
   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/i2s.h"
#include "string.h"
#include "esp_log.h"
#include "MediaHal.h"
#include "InterruptionSal.h"
#include "userconfig.h"
#include "MediaHal.h"

#include "ES8388_interface.h"
#include "ES8374_interface.h"
#include "es8311.h"
#include "codec_init.h"

#define APP_TAG "MSC_DSP"

void codec_init(void)
{
    int ret = 0;
#if (defined CONFIG_CODEC_CHIP_IS_ES8388)
    Es8388Config  Es8388Conf =  AUDIO_CODEC_ES8388_DEFAULT();
    ret = MediaHalInit(&Es8388Conf);
    if (ret) {
        ESP_LOGE(APP_TAG, "MediaHal init failed, line:%d", __LINE__);
    }
    ESP_LOGI(APP_TAG, "CONFIG_CODEC_CHIP_IS_ES8388");
#elif (defined CONFIG_CODEC_CHIP_IS_ES8374)
    Es8374Config  Es8374Conf =  AUDIO_CODEC_ES8374_DEFAULT();
    ret = MediaHalInit(&Es8374Conf);
    if (ret) {
        ESP_LOGI(APP_TAG, "MediaHal init failed, line:%d", __LINE__);
    }
    ESP_LOGI(APP_TAG, "CONFIG_CODEC_CHIP_IS_ES8374");

#elif (defined CONFIG_CODEC_CHIP_IS_ES8311)
    Es8311Config  es8311Cfg =  AUDIO_CODEC_ES8311_DEFAULT();
    ret = MediaHalInit(&es8311Cfg);
    if (ret) {
        ESP_LOGI(APP_TAG, "MediaHal init failed, line:%d", __LINE__);
    }
    ESP_LOGI(APP_TAG, "CONFIG_CODEC_CHIP_IS_ES8311");

#endif
}