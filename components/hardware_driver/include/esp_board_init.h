/**
 * 
 * @copyright Copyright 2021 Espressif Systems (Shanghai) Co. Ltd.
 *
 *      Licensed under the Apache License, Version 2.0 (the "License");
 *      you may not use this file except in compliance with the License.
 *      You may obtain a copy of the License at
 *
 *               http://www.apache.org/licenses/LICENSE-2.0
 *
 *      Unless required by applicable law or agreed to in writing, software
 *      distributed under the License is distributed on an "AS IS" BASIS,
 *      WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *      See the License for the specific language governing permissions and
 *      limitations under the License.
 */

#pragma once

#include <stdbool.h>
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Special config for dev board
 * 
 * @param sample_rate
 * @param channel_format
 * @param bits_per_chan
 * @return
 *    - ESP_OK: Success
 *    - Others: Fail
 */
esp_err_t esp_board_init(uint32_t sample_rate, int channel_format, int bits_per_chan);

/**
 * @brief Init SD crad
 * 
 * @param mount_point Path where partition should be registered (e.g. "/sdcard")
 * @param max_files Maximum number of files which can be open at the same time
 * @return
 *    - ESP_OK                  Success
 *    - ESP_ERR_INVALID_STATE   If esp_vfs_fat_register was already called
 *    - ESP_ERR_NOT_SUPPORTED   If dev board not has SDMMC/SDSPI
 *    - ESP_ERR_NO_MEM          If not enough memory or too many VFSes already registered
 *    - Others                  Fail
 */
esp_err_t esp_sdcard_init(char *mount_point, size_t max_files);

/**
 * @brief Deinit SD card
 * 
 * @param mount_point Path where partition was registered (e.g. "/sdcard")
 * @return 
 *    - ESP_OK: Success
 *    - Others: Fail
 */
esp_err_t esp_sdcard_deinit(char *mount_point);


esp_err_t get_i2s_data(char *buffer, int buffer_len);

esp_err_t esp_audio_play(const int16_t* data, int length, TickType_t ticks_to_wait);

/**
 * @brief Get the record pcm data.
 * 
 * @param is_get_raw_channel Whether to get the recording data of the original number of channels. 
 *                           Otherwise, the corresponding number of channels will be filtered based on the board.
 * @param buffer The buffer where the data is stored.
 * @param buffer_len The buffer length.
 * @return
 *    - ESP_OK                  Success
 *    - Others                  Fail
 */
esp_err_t esp_get_feed_data(bool is_get_raw_channel, int16_t *buffer, int buffer_len);

int esp_get_feed_channel(void);

char* esp_get_input_format(void);

/**
 * @brief Set play volume
 * 
 * @return
 *    - ESP_OK: Success
 *    - Others: Fail
 */
esp_err_t esp_audio_set_play_vol(int volume);

/**
 * @brief Get play volume
 * 
 * @return
 *    - ESP_OK: Success
 *    - Others: Fail
 */
esp_err_t esp_audio_get_play_vol(int *volume);


esp_err_t FatfsComboWrite(const void* buffer, int size, int count, FILE* stream);

#ifdef __cplusplus
}
#endif
