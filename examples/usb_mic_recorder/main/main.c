/* SPDX-FileCopyrightText: 2022-2023 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_wn_iface.h"
#include "esp_wn_models.h"
#include "dl_lib_coefgetter_if.h"
#include "esp_afe_sr_models.h"
#include "esp_mn_iface.h"
#include "esp_mn_models.h"
#include "esp_board_init.h"
#include "model_path.h"
#include "usb_mic_recorder.h"

static ringbuf_handle_t rb_debug = NULL;

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
    char *destry_buff = malloc(audio_chunksize * sizeof(int16_t) * feed_channel);
    assert(i2s_buff);
    assert(destry_buff);

    while (task_flag) {
        esp_get_feed_data(true, i2s_buff, audio_chunksize * sizeof(int16_t) * feed_channel);

        afe_handle->feed(afe_data, i2s_buff);

        // Write data to ringbuffer and overwrite old data if it is full.
        if (rb_bytes_available(rb_debug) < audio_chunksize * nch * sizeof(int16_t)) {
            rb_read(rb_debug, (char *) destry_buff, audio_chunksize * nch * sizeof(int16_t), 0);
        }
        rb_write(rb_debug, (char *)i2s_buff, audio_chunksize * nch * sizeof(int16_t), 0);
        
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
        afe_fetch_result_t *res = afe_handle->fetch(afe_data);
        if (res && res->ret_value != ESP_FAIL) {
            memcpy(buff, res->data, afe_chunksize * sizeof(int16_t));
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

    afe_config_t *afe_config = afe_config_init(esp_get_input_format(), NULL, AFE_TYPE_VC, AFE_MODE_LOW_COST);
    afe_handle = esp_afe_handle_from_config(afe_config);
    esp_afe_sr_data_t * afe_data = afe_handle->create_from_config(afe_config);
    afe_config_free(afe_config);
    rb_debug = mic_recorder_init();

    task_flag = 1;
    xTaskCreatePinnedToCore(&feed_Task, "feed", 8 * 1024, (void *)afe_data, 5, NULL, 0);
    xTaskCreatePinnedToCore(&detect_Task, "detect", 8 * 1024, (void *)afe_data, 5, NULL, 1);

    // You can call afe_handle->destroy to destroy AFE.
    // task_flag = 0;

    // printf("destroy\n");
    // afe_handle->destroy(afe_data);
    // afe_data = NULL;
    // printf("successful\n");
}
