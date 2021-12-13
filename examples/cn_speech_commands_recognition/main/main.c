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
#include "sdcard_init.h"
#include "esp_mn_iface.h"
#include "esp_mn_models.h"
#include "MediaHal.h"
#include "driver/i2s.h"
#include "speech_commands_action.h"
#include "model_path.h"

#if defined CONFIG_ESP32_KORVO_V1_1_BOARD || defined CONFIG_ESP32_S3_KORVO_V1_0_BOARD || defined CONFIG_ESP32_S3_KORVO_V2_0_BOARD || defined CONFIG_ESP32_S3_KORVO_V3_0_BOARD || defined CONFIG_ESP32_S3_KORVO_V4_0_BOARD || defined CONFIG_ESP32_S3_BOX_BOARD || defined CONFIG_ESP32_S3_KORVO2_V3_BOARD
#define I2S_CHANNEL_NUM 4
#else
#define I2S_CHANNEL_NUM 2
#endif

int detect_flag = 0;
static esp_afe_sr_iface_t *afe_handle = NULL;
static int play_voice = -2;

void play_music(void *arg)
{
    while (1) {
        switch (play_voice) {
        case -2:
            vTaskDelay(10);
            break;
        case -1:
            wake_up_action();
            play_voice = -2;
            break;
        default:
            speech_commands_action(play_voice);
            play_voice = -2;
            break;
        }
    }
}

void feed_Task(void *arg)
{
    esp_afe_sr_data_t *afe_data = arg;
    int audio_chunksize = afe_handle->get_feed_chunksize(afe_data);
    int nch = afe_handle->get_channel_num(afe_data);
    int16_t *i2s_buff = malloc(audio_chunksize * sizeof(int16_t) * I2S_CHANNEL_NUM);
    assert(i2s_buff);
    size_t bytes_read;
    // FILE *fp = fopen("/sdcard/out", "a+");
    // if (fp == NULL) printf("can not open file\n");

    while (1) {
        i2s_read(I2S_NUM_1, i2s_buff, audio_chunksize * I2S_CHANNEL_NUM * sizeof(int16_t), &bytes_read, portMAX_DELAY);
        // FatfsComboWrite(i2s_buff, audio_chunksize * I2S_CHANNEL_NUM * sizeof(int16_t), 1, fp);

        if (I2S_CHANNEL_NUM == 4) {
#if defined CONFIG_ESP32_S3_KORVO_V2_0_BOARD || defined CONFIG_ESP32_S3_KORVO_V3_0_BOARD  || defined CONFIG_ESP32_S3_KORVO_V4_0_BOARD || defined CONFIG_ESP32_S3_BOX_BOARD || defined CONFIG_ESP32_S3_KORVO2_V3_BOARD
            for (int i = 0; i < audio_chunksize; i++) {
                int16_t ref = i2s_buff[4 * i + 0];
                i2s_buff[3 * i + 0] = i2s_buff[4 * i + 1];
                i2s_buff[3 * i + 1] = i2s_buff[4 * i + 3];
                i2s_buff[3 * i + 2] = ref;
            }
#endif

#ifdef CONFIG_ESP32_KORVO_V1_1_BOARD
            for (int i = 0; i < audio_chunksize; i++) {
                int16_t ref = i2s_buff[4 * i + 0];
                i2s_buff[2 * i + 0] = i2s_buff[4 * i + 1];
                i2s_buff[2 * i + 1] = ref;
            }
#endif
        }

        afe_handle->feed(afe_data, i2s_buff);
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
    static const esp_mn_iface_t *multinet = &MULTINET_MODEL;
    model_iface_data_t *model_data = multinet->create(&MULTINET_COEFF, 5760);
    int mu_chunksize = multinet->get_samp_chunksize(model_data);
    int chunk_num = multinet->get_samp_chunknum(model_data);
    assert(mu_chunksize == afe_chunksize);
    printf("------------detect start------------\n");
    // FILE *fp = fopen("/sdcard/out", "w");
    // if (fp == NULL) printf("can not open file\n");
    while (1) {
        int res = afe_handle->fetch(afe_data, buff);

#if CONFIG_IDF_TARGET_ESP32
        if (res == AFE_FETCH_WWE_DETECTED) {
            printf("wakeword detected\n");
            play_voice = -1;
            detect_flag = 1;
            afe_handle->disable_wakenet(afe_data);
            printf("-----------LISTENING-----------\n");
        }
#elif CONFIG_IDF_TARGET_ESP32S3
        if (res == AFE_FETCH_WWE_DETECTED) {
            printf("wakeword detected\n");
            printf("-----------LISTENING-----------\n");
        }

        if (res == AFE_FETCH_CHANNEL_VERIFIED) {
            play_voice = -1;
            detect_flag = 1;
            afe_handle->disable_wakenet(afe_data);
        } 
#endif

        if (detect_flag == 1) {
            int command_id = multinet->detect(model_data, buff);
            // FatfsComboWrite(buff, afe_chunksize * sizeof(int16_t), 1, fp);

            if (command_id >= -2) {
                if (command_id > -1) {
                    play_voice = command_id;
                    printf("command_id: %d\n", command_id);
#ifndef CONFIG_SR_MN_CN_MULTINET3_CONTINUOUS_RECOGNITION
                    afe_handle->enable_wakenet(afe_data);
                    detect_flag = 0;
                    printf("\n-----------awaits to be waken up-----------\n");
#endif
                }

                if (command_id == -2) {
                    afe_handle->enable_wakenet(afe_data);
                    detect_flag = 0;
                    printf("\n-----------awaits to be waken up-----------\n");
                }
            }
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
    sd_card_mount("/sdcard");
    codec_init();
#if defined CONFIG_ESP_LYRAT_MINI_V1_1_BOARD || defined CONFIG_ESP32_KORVO_V1_1_BOARD
    led_init();
#endif

#if CONFIG_IDF_TARGET_ESP32
    afe_handle = &esp_afe_sr_1mic;
#else 
    afe_handle = &esp_afe_sr_2mic;
#endif

    afe_config_t afe_config = AFE_CONFIG_DEFAULT();
#if defined CONFIG_ESP32_S3_BOX_BOARD
    afe_config.aec_init = false;
    afe_config.vad_init = false;
#endif
    esp_afe_sr_data_t *afe_data = afe_handle->create_from_config(&afe_config);

    xTaskCreatePinnedToCore(&feed_Task, "feed", 4 * 1024, (void*)afe_data, 5, NULL, 0);
    xTaskCreatePinnedToCore(&detect_Task, "detect", 8 * 1024, (void*)afe_data, 5, NULL, 1);

#if defined  CONFIG_ESP32_S3_KORVO_V4_0_BOARD
    xTaskCreatePinnedToCore(&led_Task, "led", 2 * 1024, NULL, 5, NULL, 0);
#endif
#ifndef  CONFIG_ESP32_S3_BOX_BOARD
    xTaskCreatePinnedToCore(&play_music, "play", 2 * 1024, NULL, 5, NULL, 1);
#endif
}