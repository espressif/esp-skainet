#include <stdio.h>
#include <stdlib.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "esp_wn_iface.h"
#include "esp_wn_models.h"
#include "esp_mn_iface.h"
#include "esp_mn_models.h"
#include "esp_afe_sr_iface.h"
#include "esp_afe_sr_models.h"
#include "dl_lib_coefgetter_if.h"
#include "hilexin.h"
#include "xiaoaitongxue.h"
#include "dakaidiandeng.h"
#include "nch3_dakaikongtiao.h"
#include <sys/time.h>

static const esp_wn_iface_t *wakenet = &WAKENET_MODEL;
static const model_coeff_getter_t *model_coeff_getter = &WAKENET_COEFF;
static const esp_mn_iface_t *multinet = &MULTINET_MODEL;
static const esp_afe_sr_iface_t *esp_afe = &esp_afe_sr_2mic;

void wakenetTask(void *arg)
{
    model_iface_data_t *model_data = arg;
    int frequency = wakenet->get_samp_rate(model_data);
    int audio_chunksize = wakenet->get_samp_chunksize(model_data);
    int16_t *buffer = malloc(audio_chunksize * sizeof(int16_t)*2);
    assert(buffer);

    int chunks = 0;
    struct timeval tv_start, tv_end;
    gettimeofday(&tv_start, NULL);
    uint32_t c0, c1 = 0;
    RSR(CCOUNT, c0);
    int wakeword_size=sizeof(xiaoaitongxue)>sizeof(hilexin)?sizeof(xiaoaitongxue):sizeof(hilexin);
    // int wakeword_size=sizeof(xiaoaitongxue);
    // int wakeword_size=sizeof(hilexin);

    while (1) {
        if ((chunks + 1)*audio_chunksize * sizeof(int16_t) <= wakeword_size ) {
            // memcpy(buffer, xiaoaitongxue + chunks * audio_chunksize * sizeof(int16_t), audio_chunksize * sizeof(int16_t));
            // memcpy(buffer+audio_chunksize, hilexin + chunks * audio_chunksize * sizeof(int16_t), audio_chunksize * sizeof(int16_t));
            memcpy(buffer, hilexin + chunks * audio_chunksize * sizeof(int16_t), audio_chunksize * sizeof(int16_t));
            memcpy(buffer+audio_chunksize, xiaoaitongxue + chunks * audio_chunksize * sizeof(int16_t), audio_chunksize * sizeof(int16_t));
        } else {
            break;
        }
        int r = wakenet->detect(model_data, buffer);
        if (r) {
            int ch=wakenet->get_triggered_channel(model_data);
            if (ch==0) ets_printf("hilexin detected");
            else if(ch>0) ets_printf("xiaoaitongxue detected");
            ets_printf("WN test successfully, %d\n");
        }
        chunks++;
    }
    RSR(CCOUNT, c1);
    ets_printf("cycle :%d", c1-c0);
    gettimeofday(&tv_end, NULL);
    int tv_ms=(tv_end.tv_sec-tv_start.tv_sec)*1000+(tv_end.tv_usec-tv_start.tv_usec)/1000;
    printf("Done! Took %d ms to parse %d ms worth of samples in %d iterations. CPU loading(single core):%d%%\n", 
            tv_ms, chunks*audio_chunksize*1000/frequency, chunks, (int)(100*tv_ms*1000/(chunks*audio_chunksize/frequency)));
    printf("RAM size after WakeNet detection: %d\n", heap_caps_get_free_size(MALLOC_CAP_8BIT));
    printf("TEST1 DONE\n\n");
    vTaskDelete(NULL);
}


void multinetTask(void *arg)
{
    model_iface_data_t *model_data = arg;
    int audio_chunksize = multinet->get_samp_chunksize(model_data);
    int chunk_num = multinet->get_samp_chunknum(model_data);
    int16_t *buffer = malloc(audio_chunksize * sizeof(int16_t));
    assert(buffer);
    int chunks = 0;
    while (1) {
        if ((chunks+1)*audio_chunksize*sizeof(int16_t) <= sizeof(dakaidiandeng)) {
            memcpy(buffer, dakaidiandeng+chunks*audio_chunksize*sizeof(int16_t), audio_chunksize * sizeof(int16_t));   
        } else {
            memset(buffer, 0, audio_chunksize*sizeof(int16_t));
        }
        int command_id = multinet->detect(model_data, buffer);
        chunks++;
        if (chunks == chunk_num || command_id > -1) {
            if (command_id > -1) {
                printf("MN test successfully, Commands ID: %d.\n", command_id);
            } else {
                printf("can not recognize any speech commands\n");
            }
            break;
        }
    }
    printf("TEST2 DONE\n\n");
    multinet->destroy(model_data);
    vTaskDelete(NULL);
}

//feed 2 channel near-end signal and 1 far-end signal 
void feedTask(void *arg) 
{
    esp_afe_sr_data_t *afe_data = arg;
    int audio_chunksize = esp_afe->get_samp_chunksize(afe_data);
    int nch = esp_afe->get_channel_num(afe_data);
    int16_t *buffer = malloc(audio_chunksize * sizeof(int16_t)*(nch+1));
    assert(buffer);
    int chunks = 0;

    while (1) {
        if ((chunks+1)*audio_chunksize*sizeof(int16_t) <= sizeof(nch3_dakaikongtiao)) {
            memcpy(buffer, nch3_dakaikongtiao+chunks*audio_chunksize*sizeof(int16_t)*(nch+1), audio_chunksize*sizeof(int16_t)*(nch+1));   
        } else {
            break;
        }
        
        esp_afe->feed(afe_data, buffer);
        printf("feed:%d\n", chunks++);
    }
    // printf("FEED DONE\n\n");
    vTaskDelete(NULL);

}

//fetch 1 channel enhanced signal and wakenet trigger statue
void fetchTask(void *arg)
{
    esp_afe_sr_data_t *afe_data = arg;
    int afe_chunksize = esp_afe->get_samp_chunksize(afe_data);
    int16_t *buffer = malloc(afe_chunksize * sizeof(int16_t));
    // model_iface_data_t *mn_data = multinet->create(&MULTINET_COEFF, 6000);
    // int mn_chunksize = multinet->get_samp_chunksize(mn_data);
    // int chunk_num = multinet->get_samp_chunknum(mn_data);
    int mn_flag=0;
    assert(buffer);
    int chunks = 0;

    while (1) {
        int res=esp_afe->fetch(afe_data, buffer);
        if (res>0) {
            printf("AFE  test successfully\n");
        }

        printf("fetch:%d\n", chunks++);
    }
    // printf("FETCH DONE\n\n");
    vTaskDelete(NULL);
}


void app_main()
{
    int start_size = heap_caps_get_free_size(MALLOC_CAP_8BIT);
    printf("Start free RAM size: %d\n", start_size);

    // //Initialize wakenet model
    // model_iface_data_t *wn_data = wakenet->create(model_coeff_getter, DET_MODE_2CH_90);
    // printf("WakeNet RAM size: %d\nRAM size after WakeNet init: %d\n",
    //        start_size - heap_caps_get_free_size(MALLOC_CAP_8BIT), heap_caps_get_free_size(MALLOC_CAP_8BIT));
    // xTaskCreatePinnedToCore(&wakenetTask, "wakenet", 4 * 1024, (void*)wn_data, 5, NULL, 0);
    // 

    // AFE & wakenet test
    esp_afe_sr_data_t *afe_data = esp_afe->create(SR_MODE_MEDIUM, 0);
    esp_afe->set_wakenet(afe_data, wakenet, model_coeff_getter);
    xTaskCreatePinnedToCore(&feedTask, "feed", 4 * 1024, (void*)afe_data, 5, NULL, 1);
    xTaskCreatePinnedToCore(&fetchTask, "fetch", 4 * 1024, (void*)afe_data, 5, NULL, 1);
    //vTaskDelay(3000 / portTICK_PERIOD_MS);

    //// multinet model test
    //model_iface_data_t *mn_data = multinet->create(&MULTINET_COEFF, 6000);
    //printf("MultiNet RAM size: %d\nRAM size after MultiNet init: %d\n",
    //       start_size - heap_caps_get_free_size(MALLOC_CAP_8BIT), heap_caps_get_free_size(MALLOC_CAP_8BIT));
    //xTaskCreatePinnedToCore(&multinetTask, "multinet", 4 * 1024, (void*)mn_data, 5, NULL, 1);

}
