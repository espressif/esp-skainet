/*
   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/
#include <stdio.h>
#include <stdlib.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_wn_iface.h"
#include "esp_wn_models.h"
#include "esp_afe_sr_models.h"
#include "esp_mn_iface.h"
#include "esp_mn_models.h"
#include "esp_board_init.h"
#include "model_path.h"

#include "ringbuf.h"
#include "wav_encoder.h"
#include "string.h"
#include "uart.h"

typedef enum {
    RECORDER_START = 0,        // start 
    RECORDER_PAUSE = 1,        // pause 
    RECORDER_END = 2,          // stop
    RECORDER_RECORDING = 3,    // recording 
} recorder_ctrl_t;

static int out_channel = 2;
static recorder_ctrl_t ctrl_flag = RECORDER_END;
char filename[UART_BUF_LEN];

void feed_Task(void *arg)
{
    ringbuf_handle_t audio_rb = arg;
    int audio_chunksize = 512;
    int feed_channel = esp_get_feed_channel();
    if (feed_channel == 4)
        out_channel = 3;
    int16_t *i2s_buff = malloc(audio_chunksize * sizeof(int16_t) * feed_channel);
    assert(i2s_buff);
    printf("feed_Task ...\n");

    while (1) {
        esp_get_feed_data(false, i2s_buff, audio_chunksize * sizeof(int16_t) * feed_channel);
        rb_write(audio_rb, i2s_buff, audio_chunksize * sizeof(int16_t) * out_channel, portMAX_DELAY);
    }
    free(i2s_buff);
    vTaskDelete(NULL);
}

int get_start_index()
{
    char filename[128];
    int file_idx = 0;
    sprintf(filename, "/sdcard/audio_%d.wav", file_idx);
    FILE *fp = fopen(filename, "r");
    while(fp != NULL) {
        fclose(fp);
        file_idx ++;
        sprintf(filename, "/sdcard/audio_%d.wav", file_idx);
        fp = fopen(filename, "r");
    }
    fclose(fp);
    return file_idx;
}

void save_Task(void *arg)
{
    ringbuf_handle_t audio_rb = arg;
    int audio_chunksize = 512;
    int buff_size = out_channel * audio_chunksize * sizeof(int16_t);
    int16_t *buff = malloc(buff_size);
    assert(buff);
    void *wav_encoder = NULL;
    printf("save_Task ...\n");
    
    while (1) {
        rb_read(audio_rb, buff, buff_size, portMAX_DELAY);

        if (ctrl_flag == RECORDER_RECORDING) {
            wav_encoder_run(wav_encoder, (unsigned char *)buff, buff_size);
        } else if(ctrl_flag == RECORDER_START) {
            if (strlen(filename) < 9) {
                printf("Skip %s, the length of filename is too short", filename);
                ctrl_flag = RECORDER_END;
                continue;
            }
            if (wav_encoder == NULL) {
                wav_encoder = wav_encoder_open(filename, 16000, 16, out_channel);
                // filename[0] = '\0';
            } else {
                wav_encoder_close(wav_encoder);
                wav_encoder = wav_encoder_open(filename, 16000, 16, out_channel);
                // filename[0] = '\0';
            } 
            wav_encoder_run(wav_encoder, (unsigned char *)buff, buff_size);
            ctrl_flag = RECORDER_RECORDING;
        } else if (ctrl_flag == RECORDER_END) {
            if (wav_encoder != NULL) {
                wav_encoder_close(wav_encoder);
                wav_encoder = NULL;
            }
        } else if (ctrl_flag == RECORDER_PAUSE) {
            continue;
        }
    }
    free(buff);
    buff = NULL;
    vTaskDelete(NULL);
}

recorder_ctrl_t parse_uart_text(char *text, char *filename)
{
    recorder_ctrl_t ctrl = RECORDER_END;
    char *token = strtok(text, ",");
    if (strcmp(token, "start") == 0) {
        token = strtok(NULL, ",");
        if (token != NULL) {
            ctrl = RECORDER_START;
            sprintf(filename, "/sdcard/%s", token);
        }
        printf("\nfilename:%s\n", filename);
    } else if (strcmp(token, "end") == 0) {
        ctrl = RECORDER_END;
    } else if (strcmp(token, "pause") == 0) {
        ctrl = RECORDER_PAUSE;
    } 

    return ctrl;
}


void uart_ctrl_Task(void *arg)
{
    ringbuf_handle_t uart_rb = (ringbuf_handle_t) arg;
    char uart_buff[UART_BUF_LEN];
    char in;
    int buff_size = 0;
    printf("uart_ctrl_Task ...\n");

    while (1) {
        rb_read(uart_rb, &in, 1, portMAX_DELAY);

        if(in=='\n') {
            // start to run tts
            uart_buff[buff_size] = '\0';
            ctrl_flag = parse_uart_text(uart_buff, filename);
            buff_size = 0;
        } else if(buff_size<UART_BUF_LEN) {
            // append urat buffer into data
            uart_buff[buff_size] = in;
            buff_size++;
        } else {
            printf("ERROR: out of range\n");
            buff_size = 0;
        }
    }

    vTaskDelete(NULL);
}

void app_main()
{
    ESP_ERROR_CHECK(esp_board_init(16000, 1, 16));
    ESP_ERROR_CHECK(esp_sdcard_init("/sdcard", 10));
    ringbuf_handle_t audio_rb = rb_create(1024*512, 1);
    ringbuf_handle_t uart_rb = rb_create(UART_BUF_LEN, 1);
    filename[0] = '\0';

    // recorder task
    xTaskCreatePinnedToCore(&feed_Task, "feed", 4 * 1024, (void*)audio_rb, 5, NULL, 0);
    xTaskCreatePinnedToCore(&save_Task, "save", 4 * 1024, (void*)audio_rb, 5, NULL, 0);

    // uart task
    xTaskCreatePinnedToCore(&uart_read_Task, "uart_read", 6 * 1024, (void*)uart_rb, 5, NULL, 1);
    xTaskCreatePinnedToCore(&uart_ctrl_Task, "uart_ctrl", 6 * 1024, (void*)uart_rb, 5, NULL, 1);
}
