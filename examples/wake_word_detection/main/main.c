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

int detect_flag = 0;
static esp_afe_sr_iface_t *afe_handle = NULL;

void feed_Task(void *arg)
{
    esp_afe_sr_data_t *afe_data = arg;
    int audio_chunksize = afe_handle->get_feed_chunksize(afe_data);
    int nch = afe_handle->get_channel_num(afe_data);
    int feed_channel = esp_get_feed_channel();
    int16_t *i2s_buff = malloc(audio_chunksize * sizeof(int16_t) * feed_channel);
    assert(i2s_buff);
    size_t bytes_read;

    while (1) {
        esp_get_feed_data(i2s_buff, audio_chunksize * sizeof(int16_t) * feed_channel);

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
