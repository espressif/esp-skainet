/**
 * 
 * @copyright Copyright 2021 Espressif Systems (Shanghai) Co. Ltd.
 *
 *      Licensed under the Apache License, Version 2.0 (the "License");
 *      you may not use this file except in compliance with the License.
 *      You may obtain a copy of the License at
 *
 *               http://www.apache.org/licenses/LICENSE-2.0

 *      Unless required by applicable law or agreed to in writing, software
 *      distributed under the License is distributed on an "AS IS" BASIS,
 *      WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *      See the License for the specific language governing permissions and
 *      limitations under the License.
 */
#include <stdint.h>
#include <stdio.h>
#include <unistd.h>
#include "bsp_board.h"
#include "esp_err.h"
#include "esp_vfs_fat.h"
#include "sdmmc_cmd.h"
#include "esp_board_init.h"

static const char *TAG = "hardware";

esp_err_t esp_board_init(uint32_t sample_rate, int channel_format, int bits_per_chan)
{
    return bsp_board_init(sample_rate, channel_format, bits_per_chan);
}

esp_err_t esp_sdcard_init(char *mount_point, size_t max_files)
{
    return bsp_sdcard_init(mount_point, max_files);
}

esp_err_t esp_sdcard_deinit(char *mount_point)
{
    return bsp_sdcard_deinit(mount_point);
}

esp_err_t esp_get_feed_data(bool is_get_raw_channel, int16_t *buffer, int buffer_len)
{
    return bsp_get_feed_data(is_get_raw_channel, buffer, buffer_len);
}

int esp_get_feed_channel(void)
{
    return bsp_get_feed_channel();
}

char* esp_get_input_format(void)
{
    return bsp_get_input_format();
}

esp_err_t esp_audio_play(const int16_t* data, int length, TickType_t ticks_to_wait)
{
    return bsp_audio_play(data, length, ticks_to_wait);
}

esp_err_t esp_audio_set_play_vol(int volume)
{
    return bsp_audio_set_play_vol(volume);
}

esp_err_t esp_audio_get_play_vol(int *volume)
{
    return bsp_audio_get_play_vol(volume);
}

esp_err_t FatfsComboWrite(const void* buffer, int size, int count, FILE* stream)
{
    esp_err_t res = ESP_OK;
    res = fwrite(buffer, size, count, stream);
    res |= fflush(stream);
    res |= fsync(fileno(stream));

    return res;
}