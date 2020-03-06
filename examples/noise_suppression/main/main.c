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

#define DEFAULT_VREF    1100
#define NO_OF_SAMPLES   64          //Multisampling
#define NS_FRAME_BYTES     (NS_FRAME_LENGTH_MS * 16 * 2)

static esp_adc_cal_characteristics_t *adc_chars;
static const adc_channel_t channel = ADC_CHANNEL_3;
static const adc_atten_t atten = ADC_ATTEN_11db;
static const adc_unit_t unit = ADC_UNIT_1;

static bool use_ns = 1;
void noise_suppression(void *arg)
{
    size_t bytes_read;
    size_t bytes_write;
#ifdef CONFIG_ESP32_CORVO_V1_1_BOARD
    int16_t *ns_in = malloc(NS_FRAME_BYTES * 4);
#else
    int16_t *ns_in = malloc(NS_FRAME_BYTES * 2);
#endif
    int16_t *ns_in_mono = malloc(NS_FRAME_BYTES);
    int16_t *ns_out = malloc(NS_FRAME_BYTES);
    ns_handle_t ns_inst = arg;

    while (1) {
#ifdef CONFIG_ESP_LYRAT_V4_3_BOARD
        i2s_read(I2S_NUM_0, ns_in, 2 * NS_FRAME_BYTES, &bytes_read, portMAX_DELAY);
        for (int i = 0; i < NS_FRAME_BYTES / 2; i++) {
            ns_in_mono[i] = (ns_in[2 * i] + ns_in[2 * i + 1]) / 2 ;
        }
#elif defined CONFIG_ESP_LYRAT_MINI_V1_1_BOARD
        i2s_read(I2S_NUM_1, ns_in, 2 * NS_FRAME_BYTES, &bytes_read, portMAX_DELAY);
        for (int i = 0; i < NS_FRAME_BYTES / 2; i++) {
            ns_in_mono[i] = ns_in[2 * i + 1];
        }
#elif defined CONFIG_ESP32_CORVO_V1_1_BOARD
        i2s_read(I2S_NUM_1, ns_in, 4 * NS_FRAME_BYTES, &bytes_read, portMAX_DELAY);
        for (int i = 0; i < NS_FRAME_BYTES / 2; i++) {
            ns_in_mono[i] = ns_in[4 * i];
        }        
#endif
        ns_process(ns_inst, ns_in_mono, ns_out);

#ifdef CONFIG_ESP_LYRAT_V4_3_BOARD
        uint16_t *data_out = malloc(NS_FRAME_BYTES * 2);
        if (use_ns) {
            for (int i = 0; i < NS_FRAME_BYTES / 2; i++) {
                data_out[2 * i] = ns_out[i];
                data_out[2 * i + 1] = ns_out[i];
            }
        } else {
            for (int i = 0; i < NS_FRAME_BYTES / 2; i++) {
                data_out[2 * i] = ns_in_mono[i];
                data_out[2 * i + 1] = ns_in_mono[i];
            }
        }

        i2s_write(I2S_NUM_0, (const char*) data_out, NS_FRAME_BYTES * 2, &bytes_write, portMAX_DELAY);
        free(data_out);
#elif defined CONFIG_ESP_LYRAT_MINI_V1_1_BOARD || defined CONFIG_ESP32_CORVO_V1_1_BOARD
        if (use_ns)
            i2s_write(I2S_NUM_0, (const char*) ns_out, NS_FRAME_BYTES, &bytes_write, portMAX_DELAY);
        else
            i2s_write(I2S_NUM_0, (const char*) ns_in_mono, NS_FRAME_BYTES, &bytes_write, portMAX_DELAY);
#endif
    }
    vTaskDelete(NULL);
}

void button_Task(void * arg)
{
#ifdef CONFIG_ESP_LYRAT_V4_3_BOARD
    int min_vol = 120;
    int max_vol = 160;
#elif defined CONFIG_ESP_LYRAT_MINI_V1_1_BOARD
    int min_vol = 1800;
    int max_vol = 2020;
#elif defined CONFIG_ESP32_CORVO_V1_1_BOARD
    int min_vol = 1990;
    int max_vol = 2040;
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
        if (min_vol <= voltage) {
            if ( voltage <= max_vol) {
                trigger_time = system_get_time();
                if ((trigger_time - last_trigger_time) > 500000) {
                    use_ns = 1 - use_ns;
                    last_trigger_time = trigger_time;
                    printf("NS STATE: %d\n", use_ns);
                }
            }
        }
    }
}

void app_main()
{
    codec_init();
    printf("NS STATE: 1\n");
    ns_handle_t ns_inst = ns_create(NS_FRAME_LENGTH_MS);
    xTaskCreatePinnedToCore(&noise_suppression, "rec", 2 * 1024, (ns_handle_t)ns_inst, 8, NULL, 0);
    xTaskCreatePinnedToCore(&button_Task, "key", 2 * 1024, NULL, 8, NULL, 0);
}
