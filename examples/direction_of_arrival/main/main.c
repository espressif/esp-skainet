/*
   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wincompatible-pointer-types"
#pragma GCC diagnostic ignored "-Wimplicit-function-declaration"
#pragma GCC diagnostic ignored "-Wint-conversion"
#pragma GCC diagnostic ignored "-Wreturn-mismatch"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
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
#include "model_path.h"
#include "ringbuf.h"
#include "esp_nsn_models.h"
#include "model_path.h"
#include "esp_doa.h"

#define DEBUG_SAVE_PCM      1

#if DEBUG_SAVE_PCM
#define FILES_MAX           1
ringbuf_handle_t rb_debug[FILES_MAX] = {NULL};
FILE * file_save[FILES_MAX] = {NULL};
#endif

int detect_flag = 0;
static esp_afe_sr_iface_t *afe_handle = NULL;
static volatile int task_flag = 0;

void feed_Task(void *arg)
{
    int fs = 16000;
    float resolution = 20.f;
    float d_mics = 0.065f;
    float fdoa = 0.0f;    
    esp_afe_sr_data_t *afe_data = arg;
    int audio_chunksize = afe_handle->get_feed_chunksize(afe_data);
    doa_handle_t *doa_handle = esp_doa_create(fs, resolution, d_mics, audio_chunksize);
    int nch = afe_handle->get_feed_channel_num(afe_data);
    int feed_channel = esp_get_feed_channel();
    //printf("feed channel %d\n", feed_channel);
    assert(nch == feed_channel);
    int16_t *i2s_buff = malloc(audio_chunksize * sizeof(int16_t) * feed_channel);
    int16_t *ileft = malloc(audio_chunksize * sizeof(int16_t));
    int16_t *iright = malloc(audio_chunksize * sizeof(int16_t));
    assert(i2s_buff);


    char* str= esp_get_input_format();
    int length = nch;
    int positions[10]; // 假设字符串长度不会超过10
    int count = 0;

    for (int i = 0; i < length; i++) {
        if (str[i] == 'M') {
            positions[count++] = i;
        }
    }
    //printf("Positions: %d, %d\n", positions[0], positions[1]);
    while (task_flag) {
        esp_get_feed_data(true, i2s_buff, audio_chunksize * sizeof(int16_t) * feed_channel);
        for (int i = 0; i < audio_chunksize ; i++) {
            ileft [i] = i2s_buff[i * feed_channel + positions[0]];
            iright[i] = i2s_buff[i * feed_channel + positions[1]];
        }

        // for (int i = 0; i < audio_chunksize; i++) {
        //     printf("ileft: %d, iright: %d\n", ileft[i], iright[i]);
        // }

        fdoa = esp_doa_process(doa_handle, ileft, iright);    
        printf("fdoa: %f\n", fdoa);

        //afe_handle->feed(afe_data, i2s_buff);

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
    if (ileft) {
        free(ileft);
        ileft = NULL;
    }
    if (iright) {
        free(iright);
        iright = NULL;
    }
    esp_doa_destroy(doa_handle);
    vTaskDelete(NULL);
}

// void detect_Task(void *arg)
// {
//     //doa_handle_t *doa_handle = esp_doa_create(16000, 20, 0.05, 1024); 
//     //float fdoa = 0.0f;
//     esp_afe_sr_data_t *afe_data = arg;
//     int afe_chunksize = afe_handle->get_fetch_chunksize(afe_data);
//     int16_t *buff = malloc(afe_chunksize * sizeof(int16_t));
//     assert(buff);
//     printf("------------detect start------------\n");

//     while (task_flag) {
//         afe_fetch_result_t* res = afe_handle->fetch(afe_data); 
//         if (res && res->ret_value != ESP_FAIL) {
//             memcpy(buff, res->data, afe_chunksize * sizeof(int16_t));
//         //fdoa = esp_doa_process(doa_handle, &(res->data[0]), &(res->data[1]));    
//         //printf("fdoa: %f\n", fdoa);

//         #if DEBUG_SAVE_PCM
//             if (rb_bytes_available(rb_debug[1]) < afe_chunksize * 1 * sizeof(int16_t)) {
//                 printf("ERROR! rb_debug[1] slow!!!\n");
//             }

//             rb_write(rb_debug[1], buff, afe_chunksize * 1 * sizeof(int16_t), 0);
//         #endif
//         }
//     }
//     if (buff) {
//         free(buff);
//         buff = NULL;
//     }
//     //esp_doa_destroy(doa_handle);
//     vTaskDelete(NULL);
// }

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
    afe_config_t *afe_config = afe_config_init(esp_get_input_format(), models, AFE_TYPE_SR, AFE_MODE_LOW_COST);
    printf("%s\n",esp_get_input_format());
    afe_config->aec_init = false;
    afe_config_print(afe_config);
    afe_handle = esp_afe_handle_from_config(afe_config);
    esp_afe_sr_data_t *afe_data = afe_handle->create_from_config(afe_config);
    afe_config_free(afe_config);

#if DEBUG_SAVE_PCM
    rb_debug[0] = rb_create(afe_handle->get_feed_channel_num(afe_data) * 4 * 16000 * 2, 1);   // 4s ringbuf
    file_save[0] = fopen("/sdcard/feed.pcm", "w");
    if (file_save[0] == NULL) printf("can not open file\n");

    // rb_debug[1] = rb_create(1 * 4 * 16000 * 2, 1);   // 4s ringbuf
    // file_save[1] = fopen("/sdcard/fetch.pcm", "w");
    // if (file_save[1] == NULL) printf("can not open file\n");

    xTaskCreatePinnedToCore(&debug_pcm_save_Task, "debug_pcm_save", 2 * 1024, NULL, 5, NULL, 1);
#endif

    task_flag = 1;
    xTaskCreatePinnedToCore(&feed_Task, "feed", 8 * 1024, (void*)afe_data, 5, NULL, 0);
    //xTaskCreatePinnedToCore(&detect_Task, "detect", 8 * 1024, (void*)afe_data, 5, NULL, 0);

}
