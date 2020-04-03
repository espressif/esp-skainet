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
#include "esp_mase.h"
#include "ringbuf.h"
#include "esp_wn_iface.h"
#include "esp_wn_models.h"

#define DEFAULT_VREF    1100
#define NO_OF_SAMPLES   64          //Multisampling
#define MASE_FRAME_BYTES     512

static esp_adc_cal_characteristics_t *adc_chars;
static const adc_channel_t channel = ADC_CHANNEL_3;
static const adc_atten_t atten = ADC_ATTEN_11db;
static const adc_unit_t unit = ADC_UNIT_1;

struct RingBuf *mase_rb = NULL;

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
                mase_in[i + MASE_FRAME_BYTES] = i2s_in[4 * i];
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

void wakenetTask(void *arg)
{
    esp_wn_iface_t *wakenet = &WAKENET_MODEL;
    model_coeff_getter_t *model_coeff_getter = &WAKENET_COEFF;
    model_iface_data_t *model_data = wakenet->create(model_coeff_getter, DET_MODE_90);

    int frequency = wakenet->get_samp_rate(model_data);
    int audio_chunksize = wakenet->get_samp_chunksize(model_data);
    int16_t *buffer = malloc(audio_chunksize * sizeof(int16_t));
    assert(buffer);
    unsigned int chunks = 0;
    int mn_chunks = 0;
    bool detect_flag = 0;

    while (1) {
        rb_read(mase_rb, (uint8_t *)buffer, audio_chunksize * sizeof(int16_t), portMAX_DELAY);
        int r = wakenet->detect(model_data, buffer);
        if (r) {
            float ms = (chunks * audio_chunksize * 1000.0) / frequency;
            printf("%.2f: %s DETECTED.\n", (float)ms / 1000.0, wakenet->get_word_name(model_data, r));
            detect_flag = 1;
        } 
        chunks++;
    }
    vTaskDelete(NULL);
}

void buttonTask(void *arg)
{
    int min_vol = 1990;
    int max_vol = 2040;
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
        if (min_vol <= voltage) {
            if ( voltage <= max_vol) {
                trigger_time = system_get_time();
                if ((trigger_time - last_trigger_time) > 500000) {
                    enable_mase = 1 - enable_mase;
                    last_trigger_time = trigger_time;
                    printf("MASE STATE: %d\n", enable_mase);
                }
            }
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
    mase_rb = rb_init(BUFFER_PROCESS, 8 * 1024, 1, NULL);
    printf("MASE STATE: 1\n");

    xTaskCreatePinnedToCore(&maseTask, "mase", 2 * 1024, NULL, 8, NULL, 0);
    xTaskCreatePinnedToCore(&wakenetTask, "wakenet", 2 * 1024, NULL, 8, NULL, 1);
    xTaskCreatePinnedToCore(&buttonTask, "button", 2 * 1024, NULL, 8, NULL, 0);
}
