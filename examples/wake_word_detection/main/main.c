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
#include "dl_lib_coefgetter_if.h"
#include "esp_afe_sr_iface.h"
#include "esp_afe_sr_models.h"
#include "esp_mn_iface.h"
#include "esp_mn_models.h"
#include "esp_board_init.h"
#include "driver/i2s.h"
#include "model_path.h"
#include "wav_encoder.h"
#include "string.h"

int detect_flag = 0;
static esp_afe_sr_iface_t *afe_handle = NULL;

void feed_Task(void *arg)
{
    esp_afe_sr_data_t *afe_data = arg;
    int audio_chunksize = afe_handle->get_feed_chunksize(afe_data);
    int nch = afe_handle->get_channel_num(afe_data);
    int feed_channel = esp_get_feed_channel();
    int frame_size = audio_chunksize * sizeof(int16_t) * feed_channel;
    int frame_num = 100;
    int16_t *i2s_buff = malloc(frame_size);
    int16_t *out_buff = malloc(frame_num*frame_size);
    int out_buff_front = 0;
    assert(i2s_buff);
    assert(out_buff);
    size_t bytes_read;
    void * wav_encoder = NULL;
    int idx=1;
    char wav_file[128];
    sprintf(wav_file, "/sdcard/clip_%d.wav", idx);
    if (esp_sdcard_init("/sdcard", 10) == ESP_OK) {
        wav_encoder=wav_encoder_open(wav_file, 16000, 16, 1);
    }

    while (1) {
        esp_get_feed_data(i2s_buff, frame_size);

        // copy i2s data into out_buff
        // if wake word is detected, save out_buff into sdcard 
        memcpy(out_buff + out_buff_front, i2s_buff, frame_size);
        out_buff_front = (out_buff_front + frame_size)%(frame_num*frame_size);

        afe_handle->feed(afe_data, i2s_buff);

        if (detect_flag && wav_encoder != NULL) {
            wav_encoder_run(wav_encoder, out_buff + out_buff_front, frame_num*frame_size-out_buff_front);
            wav_encoder_run(wav_encoder, out_buff, out_buff_front);
            wav_encoder_close(wav_encoder);
            idx ++;
            sprintf(wav_file, "/sdcard/clip_%d.wav", idx);
            wav_encoder = wav_encoder_open(wav_file, 16000, 16, 1);
        }
    }
    afe_handle->destroy(afe_data);
    vTaskDelete(NULL);
}

void detect_Task(void *arg)
{
    esp_afe_sr_data_t *afe_data = arg;
    int afe_chunksize = afe_handle->get_fetch_chunksize(afe_data);
    int nch = afe_handle->get_channel_num(afe_data);
    int16_t *buff = malloc(afe_chunksize * sizeof(int16_t));
    assert(buff);
    printf("------------detect start------------\n");

    while (1) {
        int res = afe_handle->fetch(afe_data, buff);

        if (res == AFE_FETCH_WWE_DETECTED) {
            printf("wakeword detected\n");
            detect_flag = 1;
            printf("-----------LISTENING-----------\n");
        }
    }
    afe_handle->destroy(afe_data);
    vTaskDelete(NULL);
}

void app_main()
{
#if defined CONFIG_MODEL_IN_SPIFFS
    srmodel_spiffs_init();
#endif
    ESP_ERROR_CHECK(esp_board_init(AUDIO_HAL_08K_SAMPLES, 1, 16));
    // ESP_ERROR_CHECK(esp_sdcard_init("/sdcard", 10));

    afe_handle = &ESP_AFE_HANDLE;
    afe_config_t afe_config = AFE_CONFIG_DEFAULT();
    esp_afe_sr_data_t *afe_data = afe_handle->create_from_config(&afe_config);
    xTaskCreatePinnedToCore(&feed_Task, "feed", 4 * 1024, (void*)afe_data, 5, NULL, 0);
    xTaskCreatePinnedToCore(&detect_Task, "detect", 4 * 1024, (void*)afe_data, 5, NULL, 1);
}
