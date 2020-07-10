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
#include "driver/i2s.h"
#include "esp_system.h"
#include "sdkconfig.h"

#include "MediaHal.h"
#include "esp_mase.h"
#include "esp_agc.h"
#include "ringbuf.h"
#include "esp_wn_iface.h"
#include "esp_wn_models.h"
#include "esp_mn_iface.h"
#include "esp_mn_models.h"
#include "ws2812.h"
#include "button.h"

#define MASE_FRAME_BYTES     512
#define AGC_FRAME_BYTES     320

struct RingBuf *mase_rb = NULL;
struct RingBuf *agc_rb = NULL;

static bool enable_mase = 1;
void maseTask(void *arg)
{
    size_t bytes_read;
    size_t bytes_write;
#ifdef CONFIG_3_MIC_CIRCULAR_ARRAY
    int nch = 3;
    void *mase_handle = mase_create(16000, MASE_FRAME_SIZE, THREE_MIC_CIRCLE, 65, WAKE_UP_ENHANCEMENT_MODE, 0);
#elif defined CONFIG_2_MIC_LINEAR_ARRAY
    int nch = 2;
    void *mase_handle = mase_create(16000, MASE_FRAME_SIZE, TWO_MIC_LINE, 65, WAKE_UP_ENHANCEMENT_MODE, 0);
#endif
    int16_t *i2s_in = malloc(MASE_FRAME_BYTES * 4);
    int16_t *mase_in = malloc(MASE_FRAME_BYTES * nch);
    int16_t *mase_out = malloc(MASE_FRAME_BYTES);

    while (1) {
        i2s_read(I2S_NUM_1, i2s_in, 4 * MASE_FRAME_BYTES, &bytes_read, portMAX_DELAY);
        for (int i = 0; i < MASE_FRAME_BYTES / 2; i++)
        {
            mase_in[i] = i2s_in[4 * i + 1];
            mase_in[i + MASE_FRAME_BYTES / 2] = i2s_in[4 * i + 3];
            if (nch == 3)
            {
                mase_in[i + MASE_FRAME_BYTES] = i2s_in[4 * i + 2];
            }
        }
        mase_process(mase_handle, mase_in, mase_out);
        if (enable_mase)
        {
            i2s_write(I2S_NUM_0, (const char*) mase_out, MASE_FRAME_BYTES, &bytes_write, portMAX_DELAY);
        }
        else
        {
            i2s_write(I2S_NUM_0, (const char*) mase_in, MASE_FRAME_BYTES, &bytes_write, portMAX_DELAY);
        }
        rb_write(mase_rb, mase_out, MASE_FRAME_BYTES, portMAX_DELAY);
    }
    vTaskDelete(NULL);
}

void agcTask(void *arg)
{
    int16_t *agc_in  = malloc(AGC_FRAME_BYTES);
    int16_t *agc_out = malloc(AGC_FRAME_BYTES);

    void *agc_handle = esp_agc_open(3, 16000);
    set_agc_config(agc_handle, 15, 1, 3);

    int _err_step = 1;
    if (0 == (agc_in && _err_step ++ && agc_out && _err_step ++ && agc_handle && _err_step ++)) {
        printf("Failed to apply for memory, err_step = %d", _err_step);
        goto _agc_init_fail;
    }
    while (1) {
        rb_read(mase_rb, (uint8_t *)agc_in, AGC_FRAME_BYTES, portMAX_DELAY);
        esp_agc_process(agc_handle, agc_in, agc_out, AGC_FRAME_BYTES / 2, 16000);
        rb_write(agc_rb, (uint8_t *)agc_out, AGC_FRAME_BYTES, portMAX_DELAY);
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

void wakenetTask(void *arg)
{
    /* WakeNet */
    esp_wn_iface_t *wakenet = &WAKENET_MODEL;
    model_coeff_getter_t *model_coeff_getter = &WAKENET_COEFF;
    model_iface_data_t *model_data = wakenet->create(model_coeff_getter, DET_MODE_90);

    /* MultiNet */
    static const esp_mn_iface_t *multinet = &MULTINET_MODEL;
    model_iface_data_t *model_data_mn = multinet->create(&MULTINET_COEFF, 4000);

    int frequency = wakenet->get_samp_rate(model_data);
    int audio_chunksize = wakenet->get_samp_chunksize(model_data);
    int chunk_num = multinet->get_samp_chunknum(model_data_mn);
    int16_t *buffer = malloc(audio_chunksize * sizeof(int16_t));
    assert(buffer);
    unsigned int chunks = 0;
    int mn_chunks = 0;
    bool detect_flag = 0;

    while (1) {
        rb_read(agc_rb, (uint8_t *)buffer, audio_chunksize * sizeof(int16_t), portMAX_DELAY);
        if (detect_flag == 0) {
            int r = wakenet->detect(model_data, buffer);
            if (r) {
                float ms = (chunks * audio_chunksize * 1000.0) / frequency;
                printf("%.2f: %s DETECTED.\n", (float)ms / 1000.0, wakenet->get_word_name(model_data, r));
                detect_flag = 1;
                wake_up_light();
                printf("-----------------LISTENING-----------------\n\n");
            }
        } else {
            int command_id = multinet->detect(model_data_mn, buffer);
            mn_chunks++;
            if (mn_chunks == chunk_num || command_id > -1) {
                mn_chunks = 0;
                detect_flag = 0;
                if (command_id > -1) {
                    printf("Commands ID: %d.\n", command_id);
                    switch (command_id)
                    {
                        case 0: white_light_on(); break;
                        case 1: red_light_on(); break;
                        case 2: yellow_light_on(); break;
                        case 3: blue_light_on(); break;
                        case 4: light_off(); break;
                    }
                } else {
                    printf("can not recognize any speech commands\n");
                    return_light_state();
                }

                printf("\n-----------awaits to be waken up-----------\n");
            }
        }
        chunks++;
    }
    vTaskDelete(NULL);
}

void buttonTask(void *arg)
{
    button_init();
    char *det = calloc(4, sizeof(char));

    while (1) {
        button_detect(det);

        if (0 == strcmp(det, "null"))
        {
            vTaskDelay(100 / portTICK_PERIOD_MS);
        }
        else
        {
            if(0 == strcmp(det, "mode"))
            {
                enable_mase = 1 - enable_mase;
                printf("MASE STATE: %d\n", enable_mase);
            }
            else if(0 == strcmp(det, "play"))
            {
                white_light_on();
            }
            else if(0 == strcmp(det, "set"))
            {
                red_light_on();
            }
            else if(0 == strcmp(det, "vol+"))
            {
                yellow_light_on();
            }
            else if(0 == strcmp(det, "vol-"))
            {
                blue_light_on();
            }
            else if(0 == strcmp(det, "rec"))
            {
                light_off();
            }
            vTaskDelay(500 / portTICK_PERIOD_MS);
        }
    }
}

void app_main()
{
#ifndef CONFIG_ESP32_KORVO_V1_1_BOARD
    /* this example only works with ESP32-Korvo */
    assert(false);
#endif

    codec_init();
    init_ws2812();
    mase_rb = rb_init(BUFFER_PROCESS, 8 * 1024, 1, NULL);
    agc_rb = rb_init(BUFFER_PROCESS, 8 * 1024, 1, NULL);
    printf("MASE STATE: 1\n");

    xTaskCreatePinnedToCore(&maseTask, "mase", 2 * 1024, NULL, 8, NULL, 1);
    xTaskCreatePinnedToCore(&agcTask, "agc", 2 * 1024, NULL, 8, NULL, 1);
    xTaskCreatePinnedToCore(&wakenetTask, "wakenet", 8 * 1024, NULL, 8, NULL, 0);
    xTaskCreatePinnedToCore(&buttonTask, "button", 2 * 1024, NULL, 8, NULL, 1);
}
