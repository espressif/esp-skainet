#include "stdio.h"
#include "stdlib.h"
#include "esp_skainet_player.h"
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "wav_decoder.h"
#include "freertos/queue.h"
#include <sys/stat.h>
#include "esp_board_init.h"
#include <dirent.h>

typedef struct
{
    QueueHandle_t player_queue;
    int rb_size;
    int frame_size;
    char **file_list;
    int file_num;
    int max_file_num;
    int player_state;
    int vol;
    TaskHandle_t stream_in;
    TaskHandle_t stream_out;
    int codec_channels;
    int codec_bits_per_sample;
    int codec_sample_rate;
} esp_skainet_player_handle_t;

/* Step 1: channel convert in-place (16-bit only). wav_data is overwritten with codec_channels layout.
 * No extra buffer: 2->1 iterate back to front; 1->2 iterate back to front. */
static void wav_channel_convert_inplace(int16_t *buf, int samples, int wav_channels, int codec_channels)
{
    if (wav_channels == codec_channels) {
        return;
    }
    if (wav_channels == 2 && codec_channels == 1) {
        /* 2->1: mix L,R to mono, write to buf[0..samples-1], read from buf[0..2*samples-1] */
        for (int i = 0; i < samples; i++) {
            buf[i] = buf[2 * i];
        }
    } else {
        /* 1->2: duplicate mono to L,R, write to buf[0..2*samples-1], read from buf[0..samples-1] */
        for (int i = samples - 1; i >= 0; i--) {
            int16_t v = buf[i];
            buf[2 * i] = v;
            buf[2 * i + 1] = v;
        }
    }
}

/* Step 2: bits convert. Read 16-bit (codec_channels layout) from in, write to codec_data. For 16->32: out[i] = ret << 16. */
static void wav_bits_convert(const int16_t *in, int samples, int channels, int codec_bits, void *codec_data)
{
    if (codec_bits == 32) {
        int32_t *out = (int32_t *)codec_data;
        int n = samples * channels;
        for (int i = 0; i < n; i++) {
            out[i] = (int32_t)in[i] << 16;
        }
    } else {
        int16_t *out = (int16_t *)codec_data;
        int n = samples * channels;
        for (int i = 0; i < n; i++) {
            out[i] = in[i];
        }
    }
}

/* Convert WAV (16-bit only) to codec format. Step 1: channel convert in-place on wav_data;
 * Step 2: bits convert from wav_data to codec_data. No malloc, only input buffers. */
static int wav_to_codec_convert(const unsigned char *wav_data, int wav_len,
                                int wav_channels,
                                unsigned char *codec_data, int codec_max_len,
                                int codec_channels, int codec_bits)
{
    if (wav_channels != 1 && wav_channels != 2) {
        return -1;
    }
    if (codec_channels != 1 && codec_channels != 2) {
        return -1;
    }
    if (codec_bits != 16 && codec_bits != 32) {
        return -1;
    }

    int codec_bytes_per_sample = codec_bits / 8;
    int wav_samples_total = wav_len / (2 * wav_channels);
    int codec_samples_per_frame = codec_max_len / (codec_bytes_per_sample * codec_channels);
    int samples = wav_samples_total < codec_samples_per_frame ? wav_samples_total : codec_samples_per_frame;
    if (samples <= 0) {
        return 0;
    }

    /* Step 1: channel convert in-place in wav_data (overwrites input) */
    wav_channel_convert_inplace((int16_t *)wav_data, samples, wav_channels, codec_channels);

    /* Step 2: bits convert from wav_data to codec_data */
    wav_bits_convert((const int16_t *)wav_data, samples, codec_channels, codec_bits, codec_data);

    return samples * codec_bytes_per_sample * codec_channels;
}

void esp_skainet_stream_in_task(void *arg)
{
    esp_skainet_player_handle_t *player = arg;
    unsigned char *buffer = malloc(player->frame_size);
    unsigned char *raw_buffer = malloc((size_t)player->frame_size * 4);
    void *wav_decoder = NULL;
    int cur_file_num = 0;
    int wav_channels = 0;
    int wav_bits_per_sample = 0;
    int need_convert = 0;

    printf("create stream in\n");

    if (buffer == NULL || raw_buffer == NULL) {
        printf("stream_in: malloc failed\n");
        if (buffer) {
            free(buffer);
        }
        if (raw_buffer) {
            free(raw_buffer);
        }
        return;
    }

    while (1) {
        switch (player->player_state) {
        case 1: // play
            if (player->file_num <= 0) {
                printf("playlist is empty\n");
                player->player_state = 0;
            } else {
                while (wav_decoder == NULL) {
                    wav_decoder = wav_decoder_open(player->file_list[cur_file_num]);

                    if (wav_decoder == NULL) {
                        printf("can not find %s, play next song\n", player->file_list[cur_file_num]);
                        cur_file_num = (cur_file_num + 1) % player->file_num;
                    } else {
                        wav_channels = wav_decoder_get_channel(wav_decoder);
                        int sample_rate = wav_decoder_get_sample_rate(wav_decoder);
                        wav_bits_per_sample = wav_decoder_get_bits_per_sample(wav_decoder);

                        if (sample_rate != player->codec_sample_rate) {
                            printf("error: %s sample_rate %d != codec %d, skip\n",
                                   player->file_list[cur_file_num], sample_rate, player->codec_sample_rate);
                            wav_decoder_close(wav_decoder);
                            wav_decoder = NULL;
                            cur_file_num = (cur_file_num + 1) % player->file_num;
                            continue;
                        }
                        if (wav_channels != 1 && wav_channels != 2) {
                            printf("error: %s unsupported channels %d (only 1,2)\n",
                                   player->file_list[cur_file_num], wav_channels);
                            wav_decoder_close(wav_decoder);
                            wav_decoder = NULL;
                            cur_file_num = (cur_file_num + 1) % player->file_num;
                            continue;
                        }
                        if (wav_bits_per_sample != 16) {
                            printf("error: %s unsupported bits_per_sample %d (only 16)\n",
                                   player->file_list[cur_file_num], wav_bits_per_sample);
                            wav_decoder_close(wav_decoder);
                            wav_decoder = NULL;
                            cur_file_num = (cur_file_num + 1) % player->file_num;
                            continue;
                        }

                        need_convert = (wav_channels != player->codec_channels) ||
                                      (wav_bits_per_sample != player->codec_bits_per_sample);
                        printf("start to play %s, wav ch:%d bits:%d rate:%d %s\n",
                               player->file_list[cur_file_num], wav_channels, wav_bits_per_sample,
                               sample_rate, need_convert ? "(convert)" : "");
                        cur_file_num = (cur_file_num + 1) % player->file_num;
                    }
                }

                int read_len;
                int size;
                if (!need_convert) {
                    read_len = player->frame_size;
                    size = wav_decoder_run(wav_decoder, buffer, (unsigned int)read_len);
                    if (size < read_len) {
                        if (size > 0) {
                            memset(buffer + size, 0, (size_t)(read_len - size));
                        } else {
                            memset(buffer, 0, (size_t)read_len);
                        }
                        wav_decoder_close(wav_decoder);
                        wav_decoder = NULL;
                    }
                    xQueueSend(player->player_queue, buffer, portMAX_DELAY);
                } else {
                    int codec_bytes = (player->codec_bits_per_sample / 8) * player->codec_channels;
                    int codec_samples = player->frame_size / codec_bytes;
                    int wav_bytes_per_frame = (wav_bits_per_sample / 8) * wav_channels;
                    read_len = codec_samples * wav_bytes_per_frame;
                    if (read_len > player->frame_size * 4) {
                        read_len = player->frame_size * 4;
                    }
                    size = wav_decoder_run(wav_decoder, raw_buffer, (unsigned int)read_len);
                    if (size <= 0) {
                        memset(buffer, 0, (size_t)player->frame_size);
                        wav_decoder_close(wav_decoder);
                        wav_decoder = NULL;
                    } else {
                        int out_len = wav_to_codec_convert(
                            raw_buffer, size, wav_channels,
                            buffer, player->frame_size,
                            player->codec_channels, player->codec_bits_per_sample);
                        if (out_len < 0) {
                            printf("convert error\n");
                            memset(buffer, 0, (size_t)player->frame_size);
                        } else if (out_len < player->frame_size) {
                            memset(buffer + out_len, 0, (size_t)(player->frame_size - out_len));
                        }
                        if (size < read_len) {
                            wav_decoder_close(wav_decoder);
                            wav_decoder = NULL;
                        }
                    }
                    xQueueSend(player->player_queue, buffer, portMAX_DELAY);
                }
            }
            break;
        case 2: // pause or stop
            vTaskDelay(16 / portTICK_PERIOD_MS);
            break;

        case 3: // continue
            player->player_state = 1;
            break;

        case 4: // exit
            free(buffer);
            free(raw_buffer);
            if (wav_decoder != NULL) {
                wav_decoder_close(wav_decoder);
            }
            return;

        default:
            vTaskDelay(16 / portTICK_PERIOD_MS);
        }
    }
}

void esp_skainet_stream_out_task(void *arg)
{
    esp_skainet_player_handle_t *player = arg;
    unsigned char *buffer = malloc((size_t)player->frame_size);
    unsigned char *zero_buffer = calloc((size_t)player->frame_size, 1);
    printf("create stream_out\n");
    int count = 0;
    while (1) {
        count++;
        switch (player->player_state) {
        case 1: // play
            xQueueReceive(player->player_queue, buffer, portMAX_DELAY);
            esp_audio_play((const int16_t *)buffer, player->frame_size, portMAX_DELAY);
            break;

        case 2: // pause or stop
            esp_audio_play((const int16_t *)zero_buffer, player->frame_size, portMAX_DELAY);
            break;

        case 3: // continue
            player->player_state = 1;
            // vTaskDelay(16 / portTICK_PERIOD_MS);
            break;

        case 4: // exit
            free(buffer);
            return;

        default: // exit
            // i2s_zero_dma_buffer(0);
            vTaskDelay(16 / portTICK_PERIOD_MS);

        }
    }
}

int file_list_scan(void *handle, const char *path)
{
    esp_skainet_player_handle_t *player = handle;
    struct dirent *ret;
    DIR *dir;
    dir = opendir(path);
    int path_len = strlen(path);
    if (dir != NULL) {
        while ((ret = readdir(dir)) != NULL && player->file_num < player->max_file_num) { // NULL if reach the end of directory

            if (ret->d_type != 1) // continue if d_type is not file
                continue;

            int len = strlen(ret->d_name);
            if (len > FATFS_PATH_LENGTH_MAX - path_len - 1) // continue if name is too long
                continue;

            char *suffix = ret->d_name + len - 4;

            if (strcmp(suffix, ".wav") == 0 || strcmp(suffix, ".WAV") == 0 ) {

                memset(player->file_list[player->file_num], 0, FATFS_PATH_LENGTH_MAX);
                memcpy(player->file_list[player->file_num], path, path_len);
                memcpy(player->file_list[player->file_num] + path_len, ret->d_name, len + 1);
                printf("%d -> %s\n", player->file_num, player->file_list[player->file_num]);
                player->file_num++;
            }
        }
        closedir(dir);
    } else {
        printf("opendir NULL \r\n");
    }
    return player->file_num;
}

void *esp_skainet_player_create(int ringbuf_size, unsigned int core_num,
                                int codec_channels, int codec_bits_per_sample, int codec_sample_rate)
{
    if (ringbuf_size < 1024) {
        ringbuf_size = 1024;
    } else {
        ringbuf_size = (ringbuf_size / 1024 + 1) * 1024;
    }

    if (core_num > 1) {
        core_num = 1;
    }

    if (codec_channels != 1 && codec_channels != 2) {
        printf("esp_skainet_player_create: invalid codec_channels %d (only 1,2)\n", codec_channels);
        return NULL;
    }
    if (codec_bits_per_sample != 16 && codec_bits_per_sample != 32) {
        printf("esp_skainet_player_create: invalid codec_bits_per_sample %d (only 16,32)\n", codec_bits_per_sample);
        return NULL;
    }

    esp_skainet_player_handle_t *player = malloc(sizeof(esp_skainet_player_handle_t));
    if (player == NULL) {
        return NULL;
    }

    player->frame_size = 1024;
    player->rb_size = ringbuf_size;
    player->player_queue = xQueueCreate(ringbuf_size / player->frame_size, (UBaseType_t)player->frame_size);
    player->player_state = 0;
    player->file_num = 0;
    player->max_file_num = 10;
    player->codec_channels = codec_channels;
    player->codec_bits_per_sample = codec_bits_per_sample;
    player->codec_sample_rate = codec_sample_rate;
    player->file_list = malloc(sizeof(char *) * (size_t)player->max_file_num);
    if (player->file_list == NULL) {
        free(player);
        return NULL;
    }
    for (int i = 0; i < player->max_file_num; i++) {
        player->file_list[i] = calloc(FATFS_PATH_LENGTH_MAX, sizeof(char));
        if (player->file_list[i] == NULL) {
            for (int j = 0; j < i; j++) {
                free(player->file_list[j]);
            }
            free(player->file_list);
            free(player);
            return NULL;
        }
    }

    xTaskCreatePinnedToCore(&esp_skainet_stream_in_task, "stream_in", 2 * 1024, (void *)player, 8, NULL, core_num);
    xTaskCreatePinnedToCore(&esp_skainet_stream_out_task, "stream_out", 2 * 1024, (void *)player, 8, NULL, core_num);

    return player;
}

void esp_skainet_player_play(void *handle, const char *path)
{
    esp_skainet_player_handle_t *player = handle;
    //create file list

    file_list_scan(player, path);
    // for (int i=0; i<player->file_num; i++)
    //  printf("%s\n", player->file_list[i]);

    //set player state
    player->player_state = 1;
}

void esp_skainet_player_pause(void *handle)
{
    esp_skainet_player_handle_t *player = handle;
    player->player_state = 2;
    // printf("pause\n");
}

void esp_skainet_player_continue(void *handle)
{
    esp_skainet_player_handle_t *player = handle;
    player->player_state = 3;
    // printf("play\n");
}

void esp_skainet_player_exit(void *handle)
{
    esp_skainet_player_handle_t *player = handle;
    player->player_state = 4;
}

int esp_skainet_player_get_state(void *handle)
{
    esp_skainet_player_handle_t *player = handle;
    return player->player_state;
}

void esp_skainet_player_increase_vol(void *handle)
{
    int vol = 0;
    esp_audio_get_play_vol(&vol);
    if (vol < 50) {
        vol += 3;
    } else if (vol < 70) {
        vol += 2;
    } else if (vol < 95) {
        vol += 1;
    } else {
        vol = 95;
    }
    esp_audio_set_play_vol(vol);
}

void esp_skainet_player_decrease_vol(void *handle)
{
    int vol = 65;
    esp_audio_get_play_vol(&vol);
    if (vol >= 95) {
        vol -= 1;
    } else if (vol >= 70) {
        vol -= 2;
    } else if (vol > 50) {
        vol -= 3;
    } else {
        vol = 50;
    }

    esp_audio_set_play_vol(vol);
}


