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
#include <sys/stat.h>
#include <sys/dirent.h>
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
#include "ws2812.h"
#include "esp_vad.h"


#define DEFAULT_VREF    1100
#define NO_OF_SAMPLES   64          //Multisampling
#define I2S_FRAME_SIZE     480
#define VAD_DELAY 16000/I2S_FRAME_SIZE*300  // 10min
#define SDCARD_FRAME_BYTE  1024
#define FATFS_PATH_LENGTH_MAX 256
#define MAX_COUNT 1048576 //512*1024

static esp_adc_cal_characteristics_t *adc_chars;
static const adc_channel_t channel = ADC_CHANNEL_3;
static const adc_atten_t atten = ADC_ATTEN_11db;
static const adc_unit_t unit = ADC_UNIT_1;
#ifdef CONFIG_ESP_LYRAT_V4_3_BOARD
#define LED_GPIO 22
#elif defined CONFIG_ESP_LYRAT_MINI_V1_1_BOARD
#define LED_GPIO 22
#endif

static int enable_rec = 1;
void read_Task(void *arg)
{
    struct RingBuf *rec_rb = arg;
    size_t bytes_read;
    size_t bytes_write;
#ifdef CONFIG_ESP32_KORVO_V1_1_BOARD
    int16_t *ns_in = malloc(I2S_FRAME_SIZE*sizeof(int16_t)*4);
#else
    int16_t *ns_in = malloc(I2S_FRAME_SIZE*sizeof(int16_t)*2);
#endif
    vad_handle_t vad_handle = vad_create(VAD_MODE_4, 16000, 30);
    int16_t *vad_buff = malloc(I2S_FRAME_SIZE*sizeof(int16_t));
    int delay = 0;  //10 min
    int count = 0;
    int vad_disable = 1;

    while (1) {
#ifdef CONFIG_ESP_LYRAT_V4_3_BOARD
        i2s_read(I2S_NUM_0, ns_in, I2S_FRAME_SIZE*sizeof(int16_t)*2, &bytes_read, portMAX_DELAY);
        for (int i = 0; i < I2S_FRAME_SIZE; i++) {
            ns_in[i] = (ns_in[2 * i] + ns_in[2 * i + 1]) / 2 ;
        }
        rb_write(rec_rb, ns_in, I2S_FRAME_SIZE*sizeof(int16_t), portMAX_DELAY);
#elif defined CONFIG_ESP_LYRAT_MINI_V1_1_BOARD
        i2s_read(I2S_NUM_1, ns_in, I2S_FRAME_SIZE*sizeof(int16_t)*2, &bytes_read, portMAX_DELAY);
        for (int i = 0; i < I2S_FRAME_SIZE / 2; i++) {
            ns_in[i] = ns_in[2 * i];
        }
        rb_write(rec_rb, ns_in, I2S_FRAME_SIZE*sizeof(int16_t), portMAX_DELAY);
#elif defined CONFIG_ESP32_KORVO_V1_1_BOARD
        i2s_read(I2S_NUM_1, ns_in, I2S_FRAME_SIZE*sizeof(int16_t)*4, &bytes_read, portMAX_DELAY);
        for (int i = 0; i < I2S_FRAME_SIZE; i++) {
            ns_in[2 * i + 0] = ns_in[4 * i + 1];
            ns_in[2 * i + 1] = ns_in[4 * i + 2];
            vad_buff[i] = ns_in[4 * i + 1];
        }  
        // int state = vad_process(vad_handle, vad_buff);
        // if (state>0) {
        //     count++;
        // } else if (count>0) {
        //     count--;
        // }

        // if (count>4) {
        //     delay = VAD_DELAY;
        //     count = 0;
        // }

        if (delay>0 || vad_disable) {
            rb_write(rec_rb, ns_in, I2S_FRAME_SIZE*sizeof(int16_t)*2, portMAX_DELAY);
            // delay--;
        }
        // printf("state:%d   count:%d   delay:%d\n", state, count, delay);

#endif
    }
    vTaskDelete(NULL);
}

static int file_id=0;
static int state=0;

void record_light_on()
{
    if(state==0) {
        state=1;
#if defined CONFIG_ESP_LYRAT_V4_3_BOARD || defined CONFIG_ESP_LYRAT_MINI_V1_1_BOARD
        gpio_set_level(LED_GPIO, true)
#elif defined CONFIG_ESP32_KORVO_V1_1_BOARD
        wake_up_light();
        // RGB_1s();
#endif
    }

}

void record_light_off()
{
    if(state==1) {
        state=0;
#if defined CONFIG_ESP_LYRAT_V4_3_BOARD || defined CONFIG_ESP_LYRAT_MINI_V1_1_BOARD
        gpio_set_level(LED_GPIO, false);
#elif defined CONFIG_ESP32_KORVO_V1_1_BOARD
        light_off();
#endif
    }
}

int sdcard_scan(char *path)
{
    struct dirent *ret;
    DIR *dir;
    dir = opendir(path);
    int path_len = strlen(path);
    printf("search wav files in %s\n", path);
    int file_id=0;
    if (dir != NULL)
    {

        while ((ret = readdir(dir)) != NULL)
        { // NULL if reach the end of directory

            if (ret->d_type != 1) // continue if d_type is not file
                continue;

            int len = strlen(ret->d_name);
            if (len > FATFS_PATH_LENGTH_MAX - path_len - 1) // continue if name is too long
                continue;

            char *suffix = ret->d_name + len - 4;

            if (strcmp(suffix, ".pcm") == 0 || strcmp(suffix, ".PCM") == 0)
            {
                printf("FILE%d.pcm\n", file_id);
                file_id++;
            }
        }
        closedir(dir);
    }
    else
    {
        printf("opendir NULL \r\n");
    }
    return file_id;
}


void record_Task(void *arg)
{
    struct RingBuf *rec_rb = arg;
    FILE *fp=NULL;
    char file_name[128];
    
    int count=0;
    int16_t *buffer = malloc(SDCARD_FRAME_BYTE);

    while (1) {
        if (fp==NULL && enable_rec>0) {
            sprintf(file_name, "/sdcard/file%d.pcm", file_id);
            printf("try to open file:%s\n", file_name);
            fp=fopen(file_name, "w");
            if (fp==NULL) {
                printf("reopen file:%s\n", file_name);
            } else {
                printf("open file:%s successfully\n", file_name);
            }
        } 
        if (fp==NULL) {
            record_light_on();
            vTaskDelay(30/portTICK_PERIOD_MS);
            continue;
        } else {
            record_light_off();
        }

        rb_read(rec_rb, (uint8_t *)buffer, SDCARD_FRAME_BYTE, portMAX_DELAY);
        FatfsComboWrite(buffer, SDCARD_FRAME_BYTE, 1, fp);
        count++;
        

        if (enable_rec==0 || count>=MAX_COUNT) {
            fclose(fp);
            printf("close the file\n");
            fp=NULL;
            count=0;
            file_id++;
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
    int rec_max_vol = 2499;
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
                enable_rec=!enable_rec;
                last_trigger_time = trigger_time;
                printf("REC STATE: %d\n", enable_rec);
            }
        }
        vTaskDelay(10 / portTICK_PERIOD_MS);
    }
}

void led_init(int gpio)
{
    gpio_config_t io_conf;
    io_conf.intr_type = (gpio_int_type_t) GPIO_PIN_INTR_DISABLE;
    io_conf.mode = GPIO_MODE_OUTPUT;
    io_conf.pull_up_en = (gpio_pullup_t) 1;

    uint64_t test = ((uint64_t)1 << gpio);
    io_conf.pin_bit_mask = test;
    gpio_config(&io_conf);
    gpio_set_level(gpio, false);
}

void app_main()
{
    codec_init();
#if defined CONFIG_ESP_LYRAT_V4_3_BOARD || defined CONFIG_ESP_LYRAT_MINI_V1_1_BOARD
    led_init(LED_GPIO);
#elif defined CONFIG_ESP32_KORVO_V1_1_BOARD
    init_ws2812();
    printf("INIT ws\n");
#endif

    int ret=sd_card_mount("/sdcard");
    file_id=sdcard_scan("/sdcard");
    struct RingBuf *rec_rb = rb_init(BUFFER_PROCESS, 50 * 1024, 1, NULL);
    xTaskCreatePinnedToCore(&read_Task, "read", 8 * 1024, (void*)rec_rb, 8, NULL, 0);
    xTaskCreatePinnedToCore(&record_Task, "rec", 8 * 1024, (void*)rec_rb, 8, NULL, 1);
    xTaskCreatePinnedToCore(&button_Task, "key", 8 * 1024, NULL, 8, NULL, 1);
}
