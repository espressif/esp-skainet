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

#if defined CONFIG_ESP32_KORVO_V1_1_BOARD || defined CONFIG_ESP32_S3_KORVO_V1_0_BOARD || defined CONFIG_ESP32_S3_KORVO_V2_0_BOARD || defined CONFIG_ESP32_S3_KORVO_V3_0_BOARD || defined CONFIG_ESP32_S3_KORVO_V4_0_BOARD || defined CONFIG_ESP32_S3_BOX_BOARD
#define I2S_CHANNEL_NUM 4
#else
#define I2S_CHANNEL_NUM 2
#endif

int detect_flag = 0;
static esp_afe_sr_iface_t *afe_handle = NULL;

void feed_Task(void *arg)
{
    esp_afe_sr_data_t *afe_data = arg;
    int audio_chunksize = afe_handle->get_feed_chunksize(afe_data);
    int nch = afe_handle->get_channel_num(afe_data);
    int16_t *i2s_buff = malloc(audio_chunksize * sizeof(int16_t) * I2S_CHANNEL_NUM);
    assert(i2s_buff);
    size_t bytes_read;

    while (1) {
        i2s_read(I2S_NUM_1, i2s_buff, audio_chunksize * I2S_CHANNEL_NUM * sizeof(int16_t), &bytes_read, portMAX_DELAY);

        if (I2S_CHANNEL_NUM == 4) {
#if defined CONFIG_ESP32_S3_KORVO_V2_0_BOARD || defined CONFIG_ESP32_S3_KORVO_V3_0_BOARD  || defined CONFIG_ESP32_S3_KORVO_V4_0_BOARD || defined CONFIG_ESP32_S3_BOX_BOARD
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
    printf("------------detect start------------\n");

    while (1) {
        int res = afe_handle->fetch(afe_data, buff);

        if (res == AFE_FETCH_WWE_DETECTED) {
            printf("wakeword detected\n");
            printf("-----------LISTENING-----------\n");
        }
    }
    afe_handle->destroy(afe_data);
    vTaskDelete(NULL);
}

void app_main()
{
    codec_init();

#if CONFIG_IDF_TARGET_ESP32
    afe_handle = &esp_afe_sr_1mic;
#else 
    afe_handle = &esp_afe_sr_2mic;
#endif

    afe_config_t afe_config = AFE_CONFIG_DEFAULT();
    esp_afe_sr_data_t *afe_data = afe_handle->create_from_config(&afe_config);
    xTaskCreatePinnedToCore(&feed_Task, "feed", 4 * 1024, (void*)afe_data, 5, NULL, 0);
    xTaskCreatePinnedToCore(&detect_Task, "detect", 4 * 1024, (void*)afe_data, 5, NULL, 1);
}