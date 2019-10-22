/* 
   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "esp_system.h"
#include "sdkconfig.h"

#include "MediaHal.h"
#include "esp_wn_iface.h"
#include "esp_wn_models.h"
#include "esp_mn_iface.h"
#include "esp_mn_models.h"
#include "recsrc.h"
#include "ringbuf.h"

// WakeNet
static const esp_wn_iface_t *wakenet = &WAKENET_MODEL;
static const model_coeff_getter_t *model_coeff_getter = &WAKENET_COEFF;

struct RingBuf *aec_rb = NULL;
struct RingBuf *rec_rb = NULL;

void wakenetTask(void *arg)
{
    model_iface_data_t *model_data = arg;
    int frequency = wakenet->get_samp_rate(model_data);
    int audio_chunksize = wakenet->get_samp_chunksize(model_data);
    int16_t *buffer = malloc(audio_chunksize * sizeof(int16_t));
    assert(buffer);
    int chunks = 0;
    int mn_chunks = 0;
    bool detect_flag = 0;

    while (1) {
        rb_read(rec_rb, (uint8_t *)buffer, audio_chunksize * sizeof(int16_t), portMAX_DELAY);
        int r = wakenet->detect(model_data, buffer);
        if (r) {
            float ms = (chunks * audio_chunksize * 1000.0) / frequency;
            printf("%.2f: %s DETECTED.\n", (float)ms / 1000.0, wakenet->get_word_name(model_data, r));
            detect_flag = 1;
            chunks=0;
        }
       
        chunks++;
    }
    vTaskDelete(NULL);
}

void app_main()
{
    codec_init();
    aec_rb = rb_init(BUFFER_PROCESS, 8 * 1024, 1, NULL);
    rec_rb = rb_init(BUFFER_PROCESS, 8 * 1024, 1, NULL);

    model_iface_data_t *model_data = wakenet->create(model_coeff_getter, DET_MODE_90);
    wakenet->set_det_threshold(model_data, 0.96, 1);

    xTaskCreatePinnedToCore(&recsrcTask, "rec", 2 * 1024, NULL, 8, NULL, 0);
    xTaskCreatePinnedToCore(&agcTask, "agc", 2 * 1024, NULL, 8, NULL, 0);
    xTaskCreatePinnedToCore(&wakenetTask, "wakenet", 2 * 1024, (void*)model_data, 5, NULL, 0);

    //printf("-----------awaits to be waken up-----------\n");
}
