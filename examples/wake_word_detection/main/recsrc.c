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
#include "driver/i2s.h"
#include "ringbuf.h"
#include "recsrc.h"
#include "esp_aec.h"
#include "esp_agc.h"

extern struct RingBuf *aec_rb;
extern struct RingBuf *rec_rb;

#define AEC_FRAME_BYTES     512
#define AGC_FRAME_BYTES     320

void recsrcTask(void *arg)
{
    size_t bytes_read;
    int16_t *rsp_in = malloc(AEC_FRAME_BYTES * 2);
    int16_t *aec_rec = malloc(AEC_FRAME_BYTES);
    int16_t *aec_ref = malloc(AEC_FRAME_BYTES);
    int16_t *aec_out = malloc(AEC_FRAME_BYTES);
    void *aec_handle = aec_create(16000, AEC_FRAME_LENGTH_MS, AEC_FILTER_LENGTH);

    while (1) {
#ifdef CONFIG_ESP_LYRAT_V4_3_BOARD
        i2s_read(I2S_NUM_0, rsp_in, 2 *AEC_FRAME_BYTES, &bytes_read, portMAX_DELAY);
        for (int i = 0; i < AEC_FRAME_BYTES / 2; i++) {
            // According to test results, get better speech recognition performance 
            // when MIC gain is expanded four times for lyrat v4.3.
            aec_out[i] = (rsp_in[2 * i] + rsp_in[2 * i + 1]) *2 ;
        }
#elif defined CONFIG_ESP_LYRAT_MINI_V1_1_BOARD
        i2s_read(I2S_NUM_1, rsp_in, 2 *AEC_FRAME_BYTES, &bytes_read, portMAX_DELAY);
        for (int i = 0; i < AEC_FRAME_BYTES / 2; i++) {
            aec_ref[i] = rsp_in[2 * i];
            aec_rec[i] = rsp_in[2 * i + 1];
        }
        aec_process(aec_handle, aec_rec, aec_ref, aec_out);
#endif
        rb_write(aec_rb, aec_out, AEC_FRAME_BYTES, portMAX_DELAY);
    }
}

void agcTask(void *arg)
{
    int16_t *agc_in  = malloc(AGC_FRAME_BYTES);
    int16_t *agc_out = malloc(AGC_FRAME_BYTES);

    void *agc_handle = esp_agc_open(3, 16000);
    set_agc_config(agc_handle, 15, 1, -3);

    int _err_step = 1;
    if (0 == (agc_in && _err_step ++ && agc_out && _err_step ++ && agc_handle && _err_step ++)) {
        printf("Failed to apply for memory, err_step = %d", _err_step);
        goto _agc_init_fail;
    }
    while (1) {
        rb_read(aec_rb, (uint8_t *)agc_in, AGC_FRAME_BYTES, portMAX_DELAY);
        esp_agc_process(agc_handle, agc_in, agc_out, AGC_FRAME_BYTES / 2, 16000);

        rb_write(rec_rb, (uint8_t *)agc_out, AGC_FRAME_BYTES, portMAX_DELAY);
    }
_agc_init_fail:
    if (agc_in) {
        free(agc_in);
        agc_in = NULL;
    }
    if (agc_out) {
        free(agc_out);
        agc_out = NULL;
    }
    if (agc_handle) {
        free(agc_handle);
        agc_handle = NULL;
    }
    vTaskDelete(NULL);
}
