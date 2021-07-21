#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "esp_system.h"
#include "sdkconfig.h"
#include "driver/rmt.h"
#include "driver/i2s.h"

#include "MediaHal.h"
#include "ringbuf.h"
#include "sdcard_init.h"
#include "speech_commands.h"

#if defined CONFIG_ESP32_KORVO_V1_1_BOARD || defined CONFIG_ESP32_S3_KORVO_V1_0_BOARD || defined CONFIG_ESP32_S3_KORVO_V2_0_BOARD || defined CONFIG_ESP32_S3_KORVO_V3_0_BOARD
#define I2S_CHANNEL_NUM 4
#else
#define I2S_CHANNEL_NUM 2
#endif

static struct RingBuf *save_audio_rb = NULL;

static void record_voice(void *arg)
{
    int16_t *buffer = malloc(200 * 4 * sizeof(int16_t));
    assert(buffer);
    int chunks = 0;
    FILE *fp = fopen("/sdcard/i2s", "w");
    if (fp == NULL) printf("can not open file\n");

    while (1) {
        rb_read(save_audio_rb, (uint8_t *)buffer, 200 * 4 * sizeof(int16_t), portMAX_DELAY);
        int ret = FatfsComboWrite(buffer, 200 * 4 * sizeof(int16_t), 1, fp);
    }
    vTaskDelete(NULL);
}

void i2s_Task(void *arg)
{
    int nch = 2;
    int frame_size = 200;
    size_t bytes_read;
    int16_t *i2s_buff = malloc(frame_size * I2S_CHANNEL_NUM * sizeof(int16_t));
    printf("I2S channel num:%d, frame size:%d\n", I2S_CHANNEL_NUM, frame_size);

    while (1) {
        i2s_read(I2S_NUM_1, i2s_buff, frame_size * I2S_CHANNEL_NUM * sizeof(int16_t), &bytes_read, portMAX_DELAY);

        rb_write(save_audio_rb, i2s_buff, frame_size * 4 * sizeof(int16_t), 0);
    }

    vTaskDelete(NULL);
}

void app_main()
{
    codec_init();
    sd_card_mount("/sdcard");
    MediaHalSetVolume(70);

    save_audio_rb = rb_init(BUFFER_PROCESS, 400 * 3 * 100, 1, NULL);

    xTaskCreatePinnedToCore(&i2s_Task, "i2s", 6 * 1024, NULL, 15, NULL, 0);
    xTaskCreatePinnedToCore(&record_voice, "record_voice", 4 * 1024, NULL, 15, NULL, 1);
    vTaskDelay(2000 / portTICK_RATE_MS);
    start_action();
}