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
#include "speech_commands_action.h"
#include "ws2812.h"

// WakeNet
static const esp_wn_iface_t *wakenet = &WAKENET_MODEL;
static const model_coeff_getter_t *model_coeff_getter = &WAKENET_COEFF;
// MultiNet
static const esp_mn_iface_t *multinet = &MULTINET_MODEL;
model_iface_data_t *model_data_mn = NULL;

struct RingBuf *rec_rb = NULL;
struct RingBuf *mase_rb = NULL;
struct RingBuf *ns_rb = NULL;
struct RingBuf *agc_rb = NULL;

#ifdef CONFIG_ESP_LYRAT_V4_3_BOARD
#define LED_GPIO 22
#elif defined CONFIG_ESP_LYRAT_MINI_V1_1_BOARD
#define LED_GPIO 22
#endif

void wakenetTask(void *arg)
{
    model_iface_data_t *model_data = arg;
    int frequency = wakenet->get_samp_rate(model_data);
    int audio_chunksize = wakenet->get_samp_chunksize(model_data);
    int chunk_num = multinet->get_samp_chunknum(model_data_mn);
    printf("chunk_num = %d\n", chunk_num);
    int16_t *buffer = malloc(audio_chunksize * sizeof(int16_t));
    assert(buffer);
    int chunks = 0;
    int mn_chunks = 0;
    bool detect_flag = 0;

    while (1) {
        rb_read(agc_rb, (uint8_t *)buffer, audio_chunksize * sizeof(int16_t), portMAX_DELAY);
        if (detect_flag == 0) {
            int r = wakenet->detect(model_data, buffer);
            if (r) {
                wake_up_action();
                float ms = (chunks * audio_chunksize * 1000.0) / frequency;
                printf("%.2f: %s DETECTED.\n", (float)ms / 1000.0, wakenet->get_word_name(model_data, r));
                detect_flag = 1;
                printf("-----------------LISTENING-----------------\n\n");
#if defined CONFIG_ESP_LYRAT_V4_3_BOARD || defined CONFIG_ESP_LYRAT_MINI_V1_1_BOARD
                led_on(LED_GPIO);
#elif defined CONFIG_ESP32_KORVO_V1_1_BOARD
                wake_up_light();
#endif
                vTaskDelay(260 / portTICK_PERIOD_MS);
                rb_reset(rec_rb);
                rb_reset(ns_rb);
                rb_reset(agc_rb);
            }
        } else {
            mn_chunks++;
            int command_id = multinet->detect(model_data_mn, buffer);

            if (mn_chunks == chunk_num || command_id > -1) {
                mn_chunks = 0;
                detect_flag = 0;
                if (command_id > -1) {
                    speech_commands_action(command_id);
                } else {
                    printf("can not recognize any speech commands\n");
                }
                printf("\n-----------awaits to be waken up-----------\n");
#if defined CONFIG_ESP_LYRAT_V4_3_BOARD || defined CONFIG_ESP_LYRAT_MINI_V1_1_BOARD
                led_off(LED_GPIO);
#elif defined CONFIG_ESP32_KORVO_V1_1_BOARD
                light_off();
#endif
                rb_reset(rec_rb);
                rb_reset(ns_rb);
                rb_reset(agc_rb);
            }
        }
        chunks++;
    }
    vTaskDelete(NULL);
}

void app_main()
{
    codec_init();
#if defined CONFIG_ESP_LYRAT_V4_3_BOARD || defined CONFIG_ESP_LYRAT_MINI_V1_1_BOARD
    led_init(LED_GPIO);
#elif defined CONFIG_ESP32_KORVO_V1_1_BOARD
    init_ws2812();
#endif
    rec_rb = rb_init(BUFFER_PROCESS, 8 * 1024, 1, NULL);
#ifdef CONFIG_ESP32_KORVO_V1_1_BOARD
    mase_rb = rb_init(BUFFER_PROCESS, 8 * 1024, 1, NULL);
#else
    ns_rb = rb_init(BUFFER_PROCESS, 8 * 1024, 1, NULL);
#endif
    agc_rb = rb_init(BUFFER_PROCESS, 8 * 1024, 1, NULL);

    model_iface_data_t *model_data = wakenet->create(model_coeff_getter, DET_MODE_90);
    model_data_mn = multinet->create(&MULTINET_COEFF, 6000);

    xTaskCreatePinnedToCore(&recsrcTask, "rec", 2 * 1024, NULL, 8, NULL, 1);
#ifdef CONFIG_ESP32_KORVO_V1_1_BOARD
    xTaskCreatePinnedToCore(&maseTask, "mase", 2 * 1024, NULL, 8, NULL, 1);
#else
    xTaskCreatePinnedToCore(&nsTask, "ns", 2 * 1024, NULL, 8, NULL, 1);
#endif
    xTaskCreatePinnedToCore(&agcTask, "agc", 2 * 1024, NULL, 8, NULL, 1);
    xTaskCreatePinnedToCore(&wakenetTask, "wakenet", 2 * 1024, (void*)model_data, 5, NULL, 0);

    printf("-----------awaits to be waken up-----------\n");
}
