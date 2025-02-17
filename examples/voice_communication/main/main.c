/*
   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "esp_wn_iface.h"
#include "esp_wn_models.h"
#include "dl_lib_coefgetter_if.h"
// #include "esp_afe_sr_iface.h"
#include "esp_afe_sr_models.h"
#include "esp_mn_iface.h"
#include "esp_mn_models.h"
#include "esp_board_init.h"
#include "model_path.h"
#include "ringbuf.h"
#include "esp_nsn_models.h"
#include "model_path.h"

#define DEBUG_SAVE_PCM      0

#if DEBUG_SAVE_PCM
#define FILES_MAX           3
ringbuf_handle_t rb_debug[FILES_MAX] = {NULL};
FILE * file_save[FILES_MAX] = {NULL};
#endif

int detect_flag = 0;
static esp_afe_sr_iface_t *afe_handle = NULL;
static volatile int task_flag = 0;

void feed_Task(void *arg)
{
    esp_afe_sr_data_t *afe_data = arg;
    int audio_chunksize = afe_handle->get_feed_chunksize(afe_data);
    int nch = afe_handle->get_feed_channel_num(afe_data);
    int feed_channel = esp_get_feed_channel();
    assert(nch == feed_channel);
    int16_t *i2s_buff = malloc(audio_chunksize * sizeof(int16_t) * feed_channel);
    assert(i2s_buff);

    while (task_flag) {
        esp_get_feed_data(true, i2s_buff, audio_chunksize * sizeof(int16_t) * feed_channel);

        afe_handle->feed(afe_data, i2s_buff);

    #if DEBUG_SAVE_PCM
        if (rb_bytes_available(rb_debug[0]) < audio_chunksize * nch * sizeof(int16_t)) {
            printf("ERROR! rb_debug[0] slow!!!\n");
        }

        rb_write(rb_debug[0], i2s_buff, audio_chunksize * nch * sizeof(int16_t), 0);
    #endif
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
        if (res && res->ret_value != ESP_FAIL) {
            memcpy(buff, res->data, afe_chunksize * sizeof(int16_t));

        #if DEBUG_SAVE_PCM
            if (rb_bytes_available(rb_debug[1]) < afe_chunksize * 1 * sizeof(int16_t)) {
                printf("ERROR! rb_debug[1] slow!!!\n");
            }

            rb_write(rb_debug[1], buff, afe_chunksize * 1 * sizeof(int16_t), 0);
        #endif
        }
    }
    if (buff) {
        free(buff);
        buff = NULL;
    }
    vTaskDelete(NULL);
}

#if DEBUG_SAVE_PCM
void debug_pcm_save_Task(void *arg)
{
    int size = 4 * 2 * 32 * 16;   // It's 32ms for 4 channels, 4k bytes
    int16_t *buf_temp = heap_caps_calloc(1, size, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);

    while (task_flag) {
        for (int i = 0; i < FILES_MAX; i++) {
            if (file_save[i] != NULL) {
                if (rb_bytes_filled(rb_debug[i]) > size) {
                    int ret = rb_read(rb_debug[i], buf_temp, size, 3000 / portTICK_PERIOD_MS);
                    if ((ret < 0) || (ret < size)) {
                        // ESP_LOGE(TAG, "rb_debug read error, ret: %d\n", ret);
                        vTaskDelay(10 / portTICK_PERIOD_MS);
                        continue;
                    }
                    FatfsComboWrite(buf_temp, size, 1, file_save[i]);
                }
            }
        }
        vTaskDelay(1 / portTICK_PERIOD_MS);
    }

    free(buf_temp);
    vTaskDelete(NULL);
}
#endif

void app_main()
{
    ESP_ERROR_CHECK(esp_board_init(16000, 1, 16));
    //esp_board_init(16000, 1, 16);
#if DEBUG_SAVE_PCM
    ESP_ERROR_CHECK(esp_sdcard_init("/sdcard", 10));
#endif

    srmodel_list_t *models = esp_srmodel_init("model");
    afe_config_t *afe_config = afe_config_init(esp_get_input_format(), models, AFE_TYPE_VC, AFE_MODE_LOW_COST);
    afe_handle = esp_afe_handle_from_config(afe_config);
    esp_afe_sr_data_t *afe_data = afe_handle->create_from_config(afe_config);
    afe_config_free(afe_config);

#if DEBUG_SAVE_PCM
    rb_debug[0] = rb_create(afe_handle->get_feed_channel_num(afe_data) * 4 * 16000 * 2, 1);   // 4s ringbuf
    file_save[0] = fopen("/sdcard/feed.pcm", "w");
    if (file_save[0] == NULL) printf("can not open file\n");

    rb_debug[1] = rb_create(1 * 4 * 16000 * 2, 1);   // 4s ringbuf
    file_save[1] = fopen("/sdcard/fetch.pcm", "w");
    if (file_save[1] == NULL) printf("can not open file\n");

    xTaskCreatePinnedToCore(&debug_pcm_save_Task, "debug_pcm_save", 2 * 1024, NULL, 5, NULL, 1);
#endif

    task_flag = 1;
    xTaskCreatePinnedToCore(&feed_Task, "feed", 8 * 1024, (void*)afe_data, 5, NULL, 0);
    xTaskCreatePinnedToCore(&detect_Task, "detect", 8 * 1024, (void*)afe_data, 5, NULL, 0);

}
