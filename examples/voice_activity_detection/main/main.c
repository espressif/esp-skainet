/*
   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/
#include <stdio.h>
#include <stdlib.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_wn_iface.h"
#include "esp_wn_models.h"
#include "esp_vadn_models.h"
#include "esp_afe_sr_models.h"
#include "esp_mn_iface.h"
#include "esp_mn_models.h"
#include "esp_board_init.h"
#include "model_path.h"
#include "string.h"

static esp_afe_sr_iface_t *afe_handle = NULL;
static bool sdcard_enable = true;

void feed_Task(void *arg)
{
    esp_afe_sr_data_t *afe_data = arg;
    int audio_chunksize = afe_handle->get_feed_chunksize(afe_data);
    int nch = afe_handle->get_feed_channel_num(afe_data);
    int feed_channel = esp_get_feed_channel();
    assert(nch==feed_channel);
    int16_t *i2s_buff = malloc(audio_chunksize * sizeof(int16_t) * feed_channel);
    assert(i2s_buff);

    while (1) {
        esp_get_feed_data(true, i2s_buff, audio_chunksize * sizeof(int16_t) * feed_channel);
        afe_handle->feed(afe_data, i2s_buff);
    }
    if (i2s_buff) {
        free(i2s_buff);
        i2s_buff = NULL;
    }
    vTaskDelete(NULL);
}

void detect_Task(void *arg)
{
    esp_afe_sr_data_t *afe_data = arg;
    int afe_chunksize = afe_handle->get_fetch_chunksize(afe_data);
    int16_t *buff = malloc(afe_chunksize * sizeof(int16_t));
    assert(buff);
    printf("------------vad start------------\n");


    FILE* fp=NULL;
    if (sdcard_enable) {
        fp = fopen("/sdcard/TEST.pcm", "w+");
        if(fp==NULL) {
            printf("can not open file!\n");
        }
    }

    while (1) {
        afe_fetch_result_t* res = afe_handle->fetch(afe_data); 
        if (!res || res->ret_value == ESP_FAIL) {
            printf("fetch error!\n");
            break;
        }
        printf("vad state: %s\n", res->vad_state == VAD_SILENCE ? "noise" : "speech");

        /*
        
        VAD Cache: There are two issues in the VAD settings that can cause a delay in the first frame trigger of VAD.
        1. The inherent delay of the VAD algorithm itself. VAD cannot accurately trigger speech on the first frame and may delay by 1 to 3 frames.
        2. To avoid false triggers, the VAD is triggered when the continuous trigger duration reaches the `min_speech_ms` parameter in AFE configuation.
        Due to the above two reasons, directly using the first frame trigger of VAD may cause the first word to be truncated. 
        To avoid this situation, AFE V2.0 has added a VAD cache. You can determine whether a VAD cache needs to be saved by checking the vad_cache_size
                
        */

        if (sdcard_enable) {
            // save speech data
            if (res->vad_cache_size > 0) {
                printf("Save vad cache: %d\n", res->vad_cache_size);
                FatfsComboWrite(res->vad_cache, 1, res->vad_cache_size, fp);
            }
            if (res->vad_state == VAD_SPEECH) {
                FatfsComboWrite(res->data, 1, res->data_size, fp);
            }
        }
    }
    if (buff) {
        free(buff);
        buff = NULL;
    }
    vTaskDelete(NULL);
}

void app_main()
{
    ESP_ERROR_CHECK(esp_board_init(16000, 1, 16));
    if (sdcard_enable) {
        ESP_ERROR_CHECK(esp_sdcard_init("/sdcard", 10));
    }

    srmodel_list_t *models = esp_srmodel_init("model");
    afe_config_t *afe_config = afe_config_init(esp_get_input_format(), models, AFE_TYPE_SR, AFE_MODE_LOW_COST);
    afe_config->vad_min_noise_ms = 1000;  // The minimum duration of noise or silence in ms.
    afe_config->vad_min_speech_ms = 128;  // The minimum duration of speech in ms.
    afe_config->vad_mode = VAD_MODE_1;    // The larger the mode, the higher the speech trigger probability.


    afe_handle = esp_afe_handle_from_config(afe_config);
    esp_afe_sr_data_t *afe_data = afe_handle->create_from_config(afe_config);
    afe_config_free(afe_config);
    
    xTaskCreatePinnedToCore(&feed_Task, "feed", 8 * 1024, (void*)afe_data, 5, NULL, 0);
    xTaskCreatePinnedToCore(&detect_Task, "detect", 4 * 1024, (void*)afe_data, 5, NULL, 1);
}
