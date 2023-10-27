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
#include "esp_afe_sr_models.h"
#include "esp_mn_iface.h"
#include "esp_mn_models.h"
#include "esp_board_init.h"
#include "model_path.h"
#include "string.h"

int detect_flag = 0;
static esp_afe_sr_iface_t *afe_handle = NULL;
static esp_afe_sr_data_t *afe_data = NULL;
static volatile int task_flag = 0;

void feed_Task(void *arg)
{
    esp_afe_sr_data_t *afe_data = arg;
    int audio_chunksize = afe_handle->get_feed_chunksize(afe_data);
    int nch = afe_handle->get_channel_num(afe_data);
    int feed_channel = esp_get_feed_channel();
    assert(nch<feed_channel);
    int16_t *i2s_buff = malloc(audio_chunksize * sizeof(int16_t) * feed_channel);
    assert(i2s_buff);

    while (task_flag) {
        esp_get_feed_data(false, i2s_buff, audio_chunksize * sizeof(int16_t) * feed_channel);

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
    printf("------------detect start------------\n");

    while (task_flag) {
        afe_fetch_result_t* res = afe_handle->fetch(afe_data); 
        if (!res || res->ret_value == ESP_FAIL) {
            printf("fetch error!\n");
            break;
        }

        if (res->wakeup_state == WAKENET_DETECTED) {
            printf("wakeword detected\n");
	        printf("model index:%d, word index:%d\n", res->wakenet_model_index, res->wake_word_index);
            printf("-----------LISTENING-----------\n");
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
    // ESP_ERROR_CHECK(esp_sdcard_init("/sdcard", 10));

    srmodel_list_t *models = esp_srmodel_init("model");
    char *wn_name = NULL;
    char *wn_name_2 = NULL;

    if (models!=NULL) {
        for (int i=0; i<models->num; i++) {
            if (strstr(models->model_name[i], ESP_WN_PREFIX) != NULL) {
                if (wn_name == NULL) {
                    wn_name = models->model_name[i];
                    printf("The first wakenet model: %s\n", wn_name);
                } else if (wn_name_2 == NULL) {
                    wn_name_2 = models->model_name[i];
                    printf("The second wakenet model: %s\n", wn_name_2);
                }
            }
        }
    } else {
        printf("Please enable wakenet model and select wake word by menuconfig!\n");
        return ;
    }

    afe_handle = (esp_afe_sr_iface_t *)&ESP_AFE_SR_HANDLE;
    afe_config_t afe_config = AFE_CONFIG_DEFAULT();
    afe_config.memory_alloc_mode = AFE_MEMORY_ALLOC_MORE_PSRAM;
    afe_config.wakenet_init = true;
    afe_config.wakenet_model_name = wn_name;
    afe_config.wakenet_model_name_2 = wn_name_2;
    afe_config.voice_communication_init = false;

#if defined CONFIG_ESP32_S3_BOX_BOARD || defined CONFIG_ESP32_S3_EYE_BOARD
    afe_config.aec_init = false;
    #if defined CONFIG_ESP32_S3_EYE_BOARD
        afe_config.pcm_config.total_ch_num = 2;
        afe_config.pcm_config.mic_num = 1;
        afe_config.pcm_config.ref_num = 1;
    #endif
#endif
    afe_data = afe_handle->create_from_config(&afe_config);
    
    task_flag = 1;
    xTaskCreatePinnedToCore(&feed_Task, "feed", 8 * 1024, (void*)afe_data, 5, NULL, 0);
    xTaskCreatePinnedToCore(&detect_Task, "detect", 4 * 1024, (void*)afe_data, 5, NULL, 1);

    // // You can call afe_handle->destroy to destroy AFE.
    // task_flag = 0;

    // printf("destroy\n");
    // afe_handle->destroy(afe_data);
    // afe_data = NULL;
    // printf("successful\n");
}
