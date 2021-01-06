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
#include "driver/gpio.h"
#include "driver/adc.h"
#include "esp_adc_cal.h"

#include "MediaHal.h"
#include "esp_ns.h"
#include "esp_agc.h"
#include "sdcard_init.h"
#include "esp_err.h"
#include "wav_encoder.h"
#include "ringbuf.h"

#define DEFAULT_VREF    1100
#define NO_OF_SAMPLES   64          //Multisampling
#define NS_FRAME_BYTES     (NS_FRAME_LENGTH_MS * 16 * 2)

static esp_adc_cal_characteristics_t *adc_chars;
static const adc_channel_t channel = ADC_CHANNEL_3;
static const adc_atten_t atten = ADC_ATTEN_11db;
static const adc_unit_t unit = ADC_UNIT_1;

static bool enable_ns = 1;
static bool enable_rec = 1;
void noise_suppression(void *arg)
{
    struct RingBuf *rec_rb = arg;
    size_t bytes_read;
    size_t bytes_write;
#ifdef CONFIG_ESP32_KORVO_V1_1_BOARD
    int16_t *ns_in = malloc(NS_FRAME_BYTES * 4);
#else
    int16_t *ns_in = malloc(NS_FRAME_BYTES * 2);
#endif
    int16_t *ns_in_mono = malloc(NS_FRAME_BYTES);
    int16_t *ns_out = malloc(NS_FRAME_BYTES);
    int16_t *agc_out = malloc(NS_FRAME_BYTES);
    ns_handle_t ns_inst = ns_pro_create(NS_FRAME_LENGTH_MS, 3);

    void *agc_inst = esp_agc_open(3, 16000);
    set_agc_config(agc_inst, 10, 1, 2);

    while (1) {
#ifdef CONFIG_ESP_LYRAT_V4_3_BOARD
        i2s_read(I2S_NUM_0, ns_in, 2 * NS_FRAME_BYTES, &bytes_read, portMAX_DELAY);
        for (int i = 0; i < NS_FRAME_BYTES / 2; i++) {
            ns_in_mono[i] = (ns_in[2 * i] + ns_in[2 * i + 1]) / 2 ;
        }
#elif defined CONFIG_ESP_LYRAT_MINI_V1_1_BOARD
        i2s_read(I2S_NUM_1, ns_in, 2 * NS_FRAME_BYTES, &bytes_read, portMAX_DELAY);
        for (int i = 0; i < NS_FRAME_BYTES / 2; i++) {
            ns_in_mono[i] = ns_in[2 * i];
        }
#elif defined CONFIG_ESP32_KORVO_V1_1_BOARD
        i2s_read(I2S_NUM_1, ns_in, 4 * NS_FRAME_BYTES, &bytes_read, portMAX_DELAY);
        for (int i = 0; i < NS_FRAME_BYTES / 2; i++) {
            ns_in_mono[i] = ns_in[4 * i + 1];
        }        
#endif
        if (enable_ns) {
            //enable noise suppression
            ns_process(ns_inst, ns_in_mono, ns_out);
        } else {
            //disable noise suppression
            memcpy(ns_out, ns_in_mono, NS_FRAME_BYTES);
        }
        
        //AGC
        esp_agc_process(agc_inst, ns_out, agc_out, NS_FRAME_BYTES/2, 16000);
        rb_write(rec_rb, agc_out, NS_FRAME_BYTES, portMAX_DELAY);
    }
    vTaskDelete(NULL);
}

void record_Task(void *arg)
{
    struct RingBuf *rec_rb = arg;
    int ret=sd_card_mount("/sdcard");
    void * wav_encoder=NULL;
    FILE *fp=NULL;
    if (ret == ESP_OK) {
        wav_encoder=wav_encoder_open("/sdcard/test.wav", 16000, 16, 1);
        if (wav_encoder==NULL) {
            printf("Could not open the test.wav!\n");
        } else {
            printf("Start to record: \nNS STATE: %d\n", enable_ns);
        }
    } else {
        printf("Could not find the sdcard!\n");
        vTaskDelete(NULL); 
    }
    
    int frame_size=1000;
    int count=0;
    int16_t *buffer = malloc(frame_size*sizeof(int16_t));
    while (1) {
        rb_read(rec_rb, (uint8_t *)buffer, frame_size * sizeof(int16_t), portMAX_DELAY);
        wav_encoder_run(wav_encoder, buffer, frame_size*sizeof(int16_t));
        count++;
        if (count%16==0)
            printf("record:%d s\n", count*frame_size/16000);

        if (enable_rec==0) {
            wav_encoder_close(wav_encoder);
            wav_encoder=NULL;
            printf("Stop to record, time:%d s\n", count*frame_size/16000);
            break;
        }
    }
    vTaskDelete(NULL);
}

void button_Task(void * arg)
{

#ifdef CONFIG_ESP_LYRAT_V4_3_BOARD
    int mode_min_vol = 120;
    int mode_max_vol = 160;
    int rec_min_vol = 120;
    int rec_max_vol = 160;

#elif defined CONFIG_ESP_LYRAT_MINI_V1_1_BOARD
    int mode_min_vol = 1800;
    int mode_max_vol = 2020;
    int rec_min_vol = 2300;
    int rec_max_vol = 2500;
#elif defined CONFIG_ESP32_KORVO_V1_1_BOARD
    int rec_min_vol = 2400;
    int rec_max_vol = 2450;
    int mode_min_vol = 1990;
    int mode_max_vol = 2040;
#endif
    int last_trigger_time = 0;
    int trigger_time = 0;
    adc1_config_width(ADC_WIDTH_12Bit);
    adc1_config_channel_atten(channel, atten);
    //Characterize ADC
    adc_chars = calloc(1, sizeof(esp_adc_cal_characteristics_t));
    esp_adc_cal_characterize(unit, atten, ADC_WIDTH_12Bit, DEFAULT_VREF, adc_chars);

    while (1) {
        uint32_t adc_reading = 0;
        //Multisampling
        for (int i = 0; i < NO_OF_SAMPLES; i++) {
            adc_reading += adc1_get_raw((adc1_channel_t)channel);
        }
        adc_reading /= NO_OF_SAMPLES;
        //Convert adc_reading to voltage in mV
        uint32_t voltage = esp_adc_cal_raw_to_voltage(adc_reading, adc_chars);
        // printf("%d\n", voltage);

        if (rec_min_vol <= voltage && voltage <= rec_max_vol) {
            trigger_time = esp_timer_get_time();
            if ((trigger_time - last_trigger_time) > 500000) {
                enable_rec=0;
                last_trigger_time = trigger_time;
                printf("REC STATE: %d\n", enable_rec);
            }
        }
        if (mode_min_vol <= voltage && voltage <= rec_max_vol ) {
            trigger_time = esp_timer_get_time();
            if ((trigger_time - last_trigger_time) > 500000) {
                enable_ns = 1 - enable_ns;
                last_trigger_time = trigger_time;
                printf("NS STATE: %d\n", enable_ns);
            }
        }

        vTaskDelay(10 / portTICK_PERIOD_MS);
    }
}

void app_main()
{
    codec_init();
    struct RingBuf *rec_rb = rb_init(BUFFER_PROCESS, 20 * 1024, 1, NULL);
    xTaskCreatePinnedToCore(&noise_suppression, "ns", 8 * 1024, (void*)rec_rb, 8, NULL, 0);
    xTaskCreatePinnedToCore(&record_Task, "rec", 8 * 1024, (void*)rec_rb, 8, NULL, 1);
    xTaskCreatePinnedToCore(&button_Task, "key", 2 * 1024, NULL, 8, NULL, 1);
}
