/**
 * @copyright Copyright 2021 Espressif Systems (Shanghai) Co. Ltd.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#pragma once

#include <stdbool.h>
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Codec playback format for the board (channels, sample rate, bits per sample).
 */
typedef struct {
    int channels;           /**< Number of channels (e.g. 1 or 2) */
    int sample_rate;        /**< Sample rate in Hz (e.g. 16000) */
    int bits_per_sample;    /**< Bits per sample (e.g. 16 or 32) */
} esp_player_format_t;

/**
 * @brief Initialize the dev board.
 *
 * Board audio configuration (sample rate, channel format, bits per channel)
 * is defined per board. Use esp_get_player_format() to query the format.
 *
 * @return
 *    - ESP_OK:   Success
 *    - Others:  Fail
 */
esp_err_t esp_board_init(void);

/**
 * @brief Initialize SD card and mount FAT filesystem.
 *
 * @param mount_point  Path where partition should be registered (e.g. "/sdcard").
 * @param max_files    Maximum number of files which can be open at the same time.
 * @return
 *    - ESP_OK:                 Success
 *    - ESP_ERR_INVALID_STATE:  esp_vfs_fat_register was already called
 *    - ESP_ERR_NOT_SUPPORTED:  Dev board does not have SDMMC/SDSPI
 *    - ESP_ERR_NO_MEM:         Not enough memory or too many VFSes registered
 *    - Others:                 Fail
 */
esp_err_t esp_sdcard_init(char *mount_point, size_t max_files);

/**
 * @brief Deinitialize SD card and unmount filesystem.
 *
 * @param mount_point  Path where partition was registered (e.g. "/sdcard").
 * @return
 *    - ESP_OK:   Success
 *    - Others:  Fail
 */
esp_err_t esp_sdcard_deinit(char *mount_point);

esp_err_t get_i2s_data(char *buffer, int buffer_len);

/**
 * @brief Send PCM data to codec for playback.
 *
 * @param data            Pointer to PCM buffer (int16_t or int32_t depending on codec).
 * @param length          Length in bytes.
 * @param ticks_to_wait   Max ticks to wait for write.
 * @return
 *    - ESP_OK:   Success
 *    - Others:  Fail
 */
esp_err_t esp_audio_play(const int16_t *data, int length, TickType_t ticks_to_wait);

/**
 * @brief Get recorded PCM data.
 *
 * @param is_get_raw_channel  true: original channel count; false: filtered by board.
 * @param buffer              Buffer to store the data.
 * @param buffer_len          Buffer length in bytes.
 * @return
 *    - ESP_OK:   Success
 *    - Others:  Fail
 */
esp_err_t esp_get_feed_data(bool is_get_raw_channel, int16_t *buffer, int buffer_len);

/**
 * @brief Get the record channel number.
 *
 * @return  Record channel number.
 */
int esp_get_feed_channel(void);

/**
 * @brief Get the input format string of the board (e.g. "MMR").
 *
 * @return  Input format string.
 */
char *esp_get_input_format(void);

/**
 * @brief Get the codec playback format for this board (channels, sample rate, bits per sample).
 *
 * @param format  Output structure to fill; must not be NULL.
 */
void esp_get_player_format(esp_player_format_t *format);

/**
 * @brief Set playback volume.
 *
 * @param volume  Volume value (board-dependent range).
 * @return
 *    - ESP_OK:   Success
 *    - Others:  Fail
 */
esp_err_t esp_audio_set_play_vol(int volume);

/**
 * @brief Get current playback volume.
 *
 * @param volume  Output volume value; must not be NULL.
 * @return
 *    - ESP_OK:   Success
 *    - Others:  Fail
 */
esp_err_t esp_audio_get_play_vol(int *volume);

/**
 * @brief Write data to SD card.
 *
 * @param buffer  Pointer to the data to write.
 * @param size    Size of each element to write.
 * @param count   Number of elements to write.
 * @param stream  File stream.
 * @return
 *    - ESP_OK:   Success
 *    - Others:  Fail
 */
esp_err_t esp_sdcard_write(const void *buffer, int size, int count, FILE *stream);


esp_err_t FatfsComboWrite(const void *buffer, int size, int count, FILE *stream);

#ifdef __cplusplus
}
#endif
