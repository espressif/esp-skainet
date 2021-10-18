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

#if defined CONFIG_ESP32_KORVO_V1_1_BOARD || defined CONFIG_ESP32_S3_KORVO_V1_0_BOARD || defined CONFIG_ESP32_S3_KORVO_V2_0_BOARD || defined CONFIG_ESP32_S3_KORVO_V3_0_BOARD || defined CONFIG_ESP32_S3_KORVO_V4_0_BOARD || defined CONFIG_ESP32_S3_BOX_BOARD
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
    FILE *fp = fopen("/sdcard/out", "w");
    if (fp == NULL) printf("can not open file\n");

    while (1) {
        i2s_read(I2S_NUM_1, i2s_buff, audio_chunksize * I2S_CHANNEL_NUM * sizeof(int16_t), &bytes_read, portMAX_DELAY);

        FatfsComboWrite(i2s_buff, audio_chunksize * I2S_CHANNEL_NUM * sizeof(int16_t), 1, fp);


        if (I2S_CHANNEL_NUM == 4) {
#if defined CONFIG_ESP32_S3_KORVO_V2_0_BOARD || defined CONFIG_ESP32_S3_KORVO_V3_0_BOARD  || defined CONFIG_ESP32_S3_KORVO_V4_0_BOARD || defined CONFIG_ESP32_S3_BOX_BOARD
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
    uint32_t c0,c1,c2=0;
    int count =0;
    esp_afe_sr_data_t *afe_data = arg;
    int afe_chunksize = afe_handle->get_fetch_chunksize(afe_data);
    int nch = afe_handle->get_channel_num(afe_data);
    printf("%d %d\n", afe_chunksize, nch);
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

        if (res == AFE_FETCH_WWE_DETECTED) {
            printf("wakeword detected\n");
        }
        if (res == AFE_FETCH_CHANNEL_VERIFIED) {
            detect_flag = 1;
            afe_handle->disable_wakenet(afe_data);
            afe_handle->disable_aec(afe_data);
        } 

        if (detect_flag == 1) {
            RSR(CCOUNT, c0);
            int command_id = multinet->detect(model_data, buff);
            RSR(CCOUNT, c1);
            c2 += c1-c0;
            count++;
            printf("----\n");

            if (command_id >= -2) {
                if (command_id > -1) {
                    ets_printf("command_id: %d\n", command_id);
                    afe_handle->enable_wakenet(afe_data);
                    afe_handle->enable_aec(afe_data);
                    detect_flag = 0;
                    printf("CPU:  %f\n", c2*1.0/count/240/1000/32);

// #if defined CONFIG_EN_MULTINET5_SINGLE_RECOGNITION || defined CONFIG_EN_MULTINET3_SINGLE_RECOGNITION || defined CONFIG_CN_MULTINET2_SINGLE_RECOGNITION || defined CONFIG_CN_MULTINET3_SINGLE_RECOGNITION
                    
//                     printf("\n-----------awaits to be waken up-----------\n");

//                     multinet->reset(model_data, new_commands_str, err_id);
//                     printf("err_phrase_id: %s\n", err_id);
//                     memset(err_id, 0, 100);
// #endif
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

void spiffs_init(void)
{
    #include "esp_spiffs.h"
    printf("Initializing SPIFFS\n");

    esp_vfs_spiffs_conf_t conf = {
        .base_path = "/spiffs",
        .partition_label = NULL,
        .max_files = 50,
        .format_if_mount_failed = true
    };

    // Use settings defined above to initialize and mount SPIFFS filesystem.
    // Note: esp_vfs_spiffs_register is an all-in-one convenience function.
    esp_err_t ret = esp_vfs_spiffs_register(&conf);

    if (ret != ESP_OK) {
        if (ret == ESP_FAIL) {
            printf("Failed to mount or format filesystem\n");
        } else if (ret == ESP_ERR_NOT_FOUND) {
            printf("Failed to find SPIFFS partition\n");
        } else {
            printf("Failed to initialize SPIFFS (%s)\n", esp_err_to_name(ret));
        }
        return;
    }

    size_t total = 0, used = 0;
    ret = esp_spiffs_info(NULL, &total, &used);
    if (ret != ESP_OK) {
        printf("Failed to get SPIFFS partition information (%s)\n", esp_err_to_name(ret));
    } else {
        printf("Partition size: total: %d, used: %d\n", total, used);
    }
}

void app_main()
{
    spiffs_init();
    sd_card_mount("/sdcard");
    codec_init();
    int ram_start_size = heap_caps_get_free_size(MALLOC_CAP_8BIT);
    int dram_start_size = heap_caps_get_free_size(MALLOC_CAP_INTERNAL);
#if CONFIG_IDF_TARGET_ESP32
    afe_handle = &esp_afe_sr_1mic;
#else 
    afe_handle = &esp_afe_sr_2mic;
#endif

    afe_config_t afe_config = AFE_CONFIG_DEFAULT();
    afe_config.aec_init = false;
    afe_config.vad_init = false;
    afe_config.afe_perferred_core = 0;
    afe_config.afe_mode = SR_MODE_LOW_COST;
    esp_afe_sr_data_t *afe_data = afe_handle->create_from_config(&afe_config);

    xTaskCreatePinnedToCore(&feed_Task, "feed", 4 * 1024, (void*)afe_data, 5, NULL, 0);
    xTaskCreatePinnedToCore(&detect_Task, "detect", 5 * 1024, (void*)afe_data, 5, NULL, 0);
    vTaskDelay(3000 / portTICK_PERIOD_MS);
    int ram_left_size = heap_caps_get_free_size(MALLOC_CAP_8BIT);
    int dram_left_size = heap_caps_get_free_size(MALLOC_CAP_INTERNAL);
    printf("DRAM start:%d left:%d, use:%d, PSRAM start:%d left:%d use:%d\n", dram_start_size, dram_left_size, dram_start_size-dram_left_size,
                                                                     ram_start_size, ram_left_size, ram_start_size-ram_left_size-(dram_start_size-dram_left_size));
}
