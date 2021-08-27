#include <stdio.h>
#include <stdlib.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "esp_wn_iface.h"
#include "esp_wn_models.h"
#include "dl_lib_coefgetter_if.h"
#include <sys/time.h>
#include "esp_afe_sr_iface.h"
#include "esp_afe_sr_models.h"
#include "sdcard_init.h"
#include "esp_mn_iface.h"
#include "esp_mn_models.h"
#include "MediaHal.h"
#include "driver/i2s.h"

#if defined CONFIG_ESP32_KORVO_V1_1_BOARD || defined CONFIG_ESP32_S3_KORVO_V1_0_BOARD || defined CONFIG_ESP32_S3_KORVO_V2_0_BOARD || defined CONFIG_ESP32_S3_KORVO_V3_0_BOARD
#define I2S_CHANNEL_NUM 4
#else
#define I2S_CHANNEL_NUM 2
#endif

static esp_afe_sr_iface_t *afe_handle = NULL;

void feed_Task(void *arg)
{
    esp_afe_sr_data_t *afe_data = arg;
    int audio_chunksize = afe_handle->get_feed_chunksize(afe_data);
    int nch = afe_handle->get_channel_num(afe_data);
    int16_t *i2s_buff = malloc(audio_chunksize * sizeof(int16_t) * I2S_CHANNEL_NUM);
    assert(i2s_buff);
    size_t bytes_read;
    // FILE *fp = fopen("/sdcard/out", "w");
    // if (fp == NULL) printf("can not open file\n");

    while (1) {
        i2s_read(I2S_NUM_1, i2s_buff, audio_chunksize * I2S_CHANNEL_NUM * sizeof(int16_t), &bytes_read, portMAX_DELAY);

        // FatfsComboWrite(i2s_buff, audio_chunksize * I2S_CHANNEL_NUM * sizeof(int16_t), 1, fp);


        if (I2S_CHANNEL_NUM == 4) {
#if defined CONFIG_ESP32_S3_KORVO_V2_0_BOARD || defined CONFIG_ESP32_S3_KORVO_V3_0_BOARD
            for (int i = 0; i < audio_chunksize; i++) {
                int16_t ref = i2s_buff[4 * i + 0];
                i2s_buff[3 * i + 0] = i2s_buff[4 * i + 1];
                i2s_buff[3 * i + 1] = i2s_buff[4 * i + 3];
                i2s_buff[3 * i + 2] = ref;
            }
#endif

#ifdef CONFIG_ESP32_KORVO_V1_1_BOARD
            for (int i = 0; i < audio_chunksize; i++) {
                int16_t ref = i2s_buff[4 * i + 0];
                i2s_buff[2 * i + 0] = i2s_buff[4 * i + 1];
                i2s_buff[2 * i + 1] = ref;
            }
#endif

        }

        // FatfsComboWrite(se_ref, frame_size * nch * sizeof(int16_t), 1, fp);

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
    static const esp_mn_iface_t *multinet = &MULTINET_MODEL;
    model_iface_data_t *model_data = multinet->create(&MULTINET_COEFF, 6000);
    int mu_chunksize = multinet->get_samp_chunksize(model_data);
    int chunk_num = multinet->get_samp_chunknum(model_data);
    assert(mu_chunksize == afe_chunksize);
    printf("------------detect start------------\n");
    int detect_flag = 0;
    char *err_id = calloc(100, 1);
    char *new_commands_str = "da kai dian deng,kai dian deng;guan bi dian deng,guan dian deng;guan deng;";
    while (1) {
        int res = afe_handle->fetch(afe_data, buff);
        if (res > 0) {
            detect_flag = 1;
            printf("wakeword detected\n");
            afe_handle->disable_wakenet(afe_data);
            afe_handle->disable_aec(afe_data);
        }

        if (detect_flag == 1) {
            int command_id = multinet->detect(model_data, buff);
            printf("----\n");

            if (command_id >= -2) {
                if (command_id > -1) {
                    ets_printf("command_id: %d\n", command_id);
#if defined CONFIG_EN_MULTINET5_SINGLE_RECOGNITION || defined CONFIG_EN_MULTINET3_SINGLE_RECOGNITION || defined CONFIG_CN_MULTINET2_SINGLE_RECOGNITION || defined CONFIG_CN_MULTINET3_SINGLE_RECOGNITION
                    afe_handle->enable_wakenet(afe_data);
                    afe_handle->enable_aec(afe_data);
                    detect_flag = 0;
                    printf("\n-----------awaits to be waken up-----------\n");

                    multinet->reset(model_data, new_commands_str, err_id);
                    printf("err_phrase_id: %s\n", err_id);
                    memset(err_id, 0, 100);
#endif
                }

                if (command_id == -2) {
                    afe_handle->enable_wakenet(afe_data);
                    detect_flag = 0;
                    printf("\n-----------awaits to be waken up-----------\n");
                }
            }
        }
    }
    afe_handle->destroy(afe_data);
    vTaskDelete(NULL);
}

void app_main()
{
    codec_init();
#if CONFIG_IDF_TARGET_ESP32
    afe_handle = &esp_afe_sr_1mic;
#else 
    afe_handle = &esp_afe_sr_2mic;
#endif
    const esp_wn_iface_t *wakenet = &WAKENET_MODEL;
    const model_coeff_getter_t *model_coeff_getter = &WAKENET_COEFF;

    int afe_perferred_core = 0;
    int afe_mode = SR_MODE_LOW_COST;
#ifdef CONFIG_IDF_TARGET_ESP32
    afe_mode = SR_MODE_LOW_COST;
    printf("ESP32 only support afe mode = SR_MODE_LOW_COST or SR_MODE_MEDIUM_COST\n");   
#endif
    printf("AFE mode: %d\n", afe_mode);
    esp_afe_sr_data_t *afe_data = afe_handle->create(afe_mode, afe_perferred_core);
    afe_handle->set_wakenet(afe_data, &WAKENET_MODEL, &WAKENET_COEFF);
    xTaskCreatePinnedToCore(&feed_Task, "feed", 4 * 1024, (void*)afe_data, 5, NULL, afe_perferred_core);
    xTaskCreatePinnedToCore(&detect_Task, "detect", 4 * 1024, (void*)afe_data, 5, NULL, 1);
}
