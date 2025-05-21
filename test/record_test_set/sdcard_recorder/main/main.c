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
#include <sys/stat.h>
#include <dirent.h>

#include "esp_skainet_player.h"

typedef enum {
    RECORDER_START = 0,        // start
    RECORDER_PAUSE = 1,        // pause
    RECORDER_END = 2,          // stop
    RECORDER_RECORDING = 3,    // recording
    RECORDER_PLAY = 4,         // play
} recorder_ctrl_t;

static int out_channel = 2;
static recorder_ctrl_t ctrl_flag = RECORDER_END;
char filename[UART_BUF_LEN];
static afe_pcm_config_t pcm_config;
static void *player = NULL;
static int play_flag = 0;

void get_data(int16_t *buff, int16_t *out, int size)
{
    int total_channel = pcm_config.total_ch_num;
    int mic_num = pcm_config.mic_num;
    int ref_num = pcm_config.ref_num;
    for (int i = 0; i < size; i++) {
        for (int j = 0; j < mic_num; j++) {
            out[i * out_channel + j] = buff[i*total_channel + pcm_config.mic_ids[j]];
        }
        for (int j=0; j < ref_num; j++) {
            out[i * out_channel + j + mic_num] = buff[i*total_channel + pcm_config.ref_ids[j]];
        }
    }
}

void feed_task(void *arg)
{
    ringbuf_handle_t audio_rb = arg;
    int audio_chunksize = 512;
    int feed_channel = esp_get_feed_channel();
    int16_t *i2s_buff = malloc(audio_chunksize * sizeof(int16_t) * feed_channel);
    int16_t *out_buff = malloc(audio_chunksize * sizeof(int16_t) * out_channel);
    assert(i2s_buff);

    while (1) {
        esp_get_feed_data(true, i2s_buff, audio_chunksize * sizeof(int16_t) * feed_channel);
        get_data(i2s_buff, out_buff, audio_chunksize);
        rb_write(audio_rb, (char *)out_buff, audio_chunksize * sizeof(int16_t) * out_channel, portMAX_DELAY);
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

void save_task(void *arg)
{
    ringbuf_handle_t audio_rb = arg;
    int audio_chunksize = 512;
    int buff_size = out_channel * audio_chunksize * sizeof(int16_t);
    int16_t *buff = malloc(buff_size);
    assert(buff);
    void *wav_encoder = NULL;

    while (1) {
        rb_read(audio_rb, (char *)buff, buff_size, portMAX_DELAY);

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
        play_flag = 0;
    } else if (strcmp(token, "pause") == 0) {
        ctrl = RECORDER_PAUSE;
        play_flag = 0;
    } else if (strcmp(token, "play") == 0) {
        play_flag = 1;
    }

    return ctrl;
}


void uart_ctrl_task(void *arg)
{
    ringbuf_handle_t uart_rb = (ringbuf_handle_t) arg;
    char uart_buff[UART_BUF_LEN];
    char in;
    int buff_size = 0;

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

int sdcard_scan(const char *path)
{
    struct dirent *ret;
    DIR *dir;
    dir = opendir(path);
    int path_len = strlen(path);
    printf("Search files in %s\n", path);
    int file_num = 0;
    if (dir != NULL) {

        while ((ret = readdir(dir)) != NULL) {
            // NULL if reach the end of directory

            if (ret->d_type != 1) { // continue if d_type is not file
                continue;
            }

            int len = strlen(ret->d_name);
            if (len > FATFS_PATH_LENGTH_MAX - path_len - 1) { // continue if name is too long
                continue;
            }

            char *suffix = ret->d_name + len - 4;

            if (strcmp(suffix, ".wav") == 0 || strcmp(suffix, ".WAV") == 0) {
                file_num++;
            }

        }
        closedir(dir);
        printf("Number of files: %d\n", file_num);
    } else {
        printf("Fail to open %s\r\n", path);
    }
    return file_num;
}

void play_task()
{
    //play task
    int file_num = sdcard_scan("/sdcard/music/");
    if (file_num > 0) {
        printf("start play task\n");
        //Adjust player volume
        int vol;
        esp_audio_set_play_vol(100);
        esp_audio_get_play_vol(&vol);
        printf("player volume: %d\n", vol);

        player = esp_skainet_player_create(4096 * 5, 1);
        esp_skainet_player_play(player, "/sdcard/music/");
        esp_skainet_player_pause(player);
    }
    int cur_play_state = play_flag;

    while (player) {
        if (cur_play_state != play_flag) {
            if (play_flag == 1) {
                esp_skainet_player_continue(player);
            } else {
                esp_skainet_player_pause(player);
            }

            cur_play_state = play_flag;
        }

        vTaskDelay(100 / portTICK_PERIOD_MS);
    }
    vTaskDelete(NULL);
}


void app_main()
{
    ESP_ERROR_CHECK(esp_board_init(16000, 1, 16));
    ESP_ERROR_CHECK(esp_sdcard_init("/sdcard", 10));
    ringbuf_handle_t audio_rb = rb_create(1024*512, 1);
    ringbuf_handle_t uart_rb = rb_create(UART_BUF_LEN, 1);
    // filename[0] = '\0';
    afe_parse_input_format(esp_get_input_format(), &pcm_config);
    out_channel = pcm_config.mic_num + pcm_config.ref_num;
    printf("microphone channel:%d, playback channel:%d, output channel:%d\n",
            pcm_config.mic_num, pcm_config.ref_num, out_channel);

    // play task
    xTaskCreatePinnedToCore(&play_task, "play", 4*1024, NULL, 6, NULL, 0);
    vTaskDelay(30 / portTICK_PERIOD_MS);

    // recorder task
    xTaskCreatePinnedToCore(&feed_task, "feed", 4 * 1024, (void*)audio_rb, 5, NULL, 0);
    xTaskCreatePinnedToCore(&save_task, "save", 4 * 1024, (void*)audio_rb, 5, NULL, 0);
    vTaskDelay(30 / portTICK_PERIOD_MS);

    // uart task
    xTaskCreatePinnedToCore(&uart_read_task, "uart_read", 6 * 1024, (void*)uart_rb, 5, NULL, 1);
    xTaskCreatePinnedToCore(&uart_ctrl_task, "uart_ctrl", 6 * 1024, (void*)uart_rb, 5, NULL, 1);

    // printf("-----START-----");
}
