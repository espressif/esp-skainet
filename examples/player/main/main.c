/*
 * Player example: play WAV files from SD card /sdcard/music/
 *
 * Place WAV files in the music folder on the SD card. The player will
 * scan the folder and play all .wav files in sequence.
 */
#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_board_init.h"
#include "esp_skainet_player.h"

#define MUSIC_PATH    "/sdcard/music"
#define RINGBUF_SIZE  (4096 * 5)

void app_main(void)
{
    ESP_ERROR_CHECK(esp_board_init(16000, 1, 16));
    ESP_ERROR_CHECK(esp_sdcard_init("/sdcard", 10));

    void *player = esp_skainet_player_create(RINGBUF_SIZE, 1);
    if (player == NULL) {
        printf("Failed to create player\n");
        return;
    }

    int vol = 0;
    esp_audio_get_play_vol(&vol);
    printf("Player volume: %d\n", vol);

    printf("Start playing WAV files from %s\n", MUSIC_PATH);
    esp_skainet_player_play(player, MUSIC_PATH);

    while (1) {
        int state = esp_skainet_player_get_state(player);
        if (state == 0) {
            printf("Playback finished\n");
            break;
        }
        vTaskDelay(pdMS_TO_TICKS(500));
    }

    esp_skainet_player_exit(player);
    printf("Player exit\n");
}
