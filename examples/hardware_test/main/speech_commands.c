#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "esp_system.h"
#include "sdkconfig.h"

#include "ie_kaiji.h"
#include "driver/i2s.h"

typedef struct {
    char* name;
    const uint16_t* data;
    int length;
} dac_audio_item_t;

esp_err_t iot_dac_audio_play(const uint16_t* data, int length, TickType_t ticks_to_wait)
{
    size_t bytes_write = 0;
    i2s_write(0, (const char*) data, length, &bytes_write, ticks_to_wait);

    i2s_zero_dma_buffer(I2S_NUM_0);
    return ESP_OK;
}

dac_audio_item_t playlist[] = {
    {"ie_kaiji.h", ie_kaiji, sizeof(ie_kaiji)},
};

void start_action(void)
{
    iot_dac_audio_play(playlist[0].data, playlist[0].length, portMAX_DELAY);
}