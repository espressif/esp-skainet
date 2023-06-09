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

#include "ringbuf.h"
#include "wav_encoder.h"

static int out_channel = 2;
void feed_Task(void *arg)
{
    ringbuf_handle_t audio_rb = arg;
    int audio_chunksize = 512;
    int feed_channel = esp_get_feed_channel();
    if (feed_channel == 4)
        out_channel = 3;
    int16_t *i2s_buff = malloc(audio_chunksize * sizeof(int16_t) * feed_channel);
    assert(i2s_buff);

    while (1) {
        esp_get_feed_data(false, i2s_buff, audio_chunksize * sizeof(int16_t) * feed_channel);
        rb_write(audio_rb, i2s_buff, audio_chunksize * sizeof(int16_t) * out_channel, portMAX_DELAY);
        printf("write %d \n", rb_bytes_available(audio_rb));

    }
    free(i2s_buff);
    vTaskDelete(NULL);
}

int get_start_index()
{
    char filename[128];
    int file_idx = 0;
    sprintf(filename, "/sdcard/audio_%d.wav", file_idx);
    FILE *fp = fopen(filename, "r");
    while(fp != NULL) {
        fclose(fp);
        file_idx ++;
        sprintf(filename, "/sdcard/audio_%d.wav", file_idx);
        fp = fopen(filename, "r");
    }
    fclose(fp);
    return file_idx;
}

void save_Task(void *arg)
{
    ringbuf_handle_t audio_rb = arg;
    int audio_chunksize = 512;
    int buff_size = out_channel * audio_chunksize * sizeof(int16_t);
    int16_t *buff = malloc(buff_size);
    assert(buff);
    int count = 0;
    int split_num = 3600 * 16000 / 512;  // one hour per file 
    char filename[128];
    int file_idx = get_start_index();
    sprintf(filename, "/sdcard/audio_%d.wav", file_idx);
    void *wav_encoder=wav_encoder_open(filename, 16000, 16, out_channel);
    printf("Save audio data as %s, %d\n", filename, split_num);
    
    while (1) {
        rb_read(audio_rb, buff, buff_size, portMAX_DELAY);
        count ++;
        if (count ==  split_num) {
            count = 0;
            wav_encoder_close(wav_encoder);
            file_idx ++;
            sprintf(filename, "/sdcard/audio_%d.wav", file_idx);
            wav_encoder=wav_encoder_open(filename, 16000, 16, out_channel); 
            printf("Save audio data as %s\n", filename);
        } else {
            wav_encoder_run(wav_encoder, buff, buff_size);
        }
        printf("read %d \n", rb_bytes_available(audio_rb));

    }
    free(buff);
    buff = NULL;
    vTaskDelete(NULL);
}

void app_main()
{
    ESP_ERROR_CHECK(esp_board_init(AUDIO_HAL_16K_SAMPLES, 1, 16));
    ESP_ERROR_CHECK(esp_sdcard_init("/sdcard", 10));
    ringbuf_handle_t audio_rb = rb_create(1024*512, 1);;

    xTaskCreatePinnedToCore(&feed_Task, "feed", 4 * 1024, (void*)audio_rb, 5, NULL, 0);
    xTaskCreatePinnedToCore(&save_Task, "save", 4 * 1024, (void*)audio_rb, 6, NULL, 1);
}
