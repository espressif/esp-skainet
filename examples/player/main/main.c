/*
 * Player example: play WAV files from SD card /sdcard/music/
 *
 * Place WAV files in the music folder on the SD card. Control playback
 * via console commands: play, pause, stop, vol+, vol-
 */
#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_board_init.h"
#include "esp_skainet_player.h"
#include "esp_console.h"

#define MUSIC_PATH    "/sdcard/music/"
#define RINGBUF_SIZE  (4096 * 4)

static void *player = NULL;

/* Console command handlers */
static int cmd_play(int argc, char **argv)
{
    if (player == NULL) {
        printf("Player not created\n");
        return 0;
    }
    esp_skainet_player_continue(player);
    printf("Play\n");
    return 0;
}

static int cmd_pause(int argc, char **argv)
{
    if (player == NULL) {
        printf("Player not created\n");
        return 0;
    }
    esp_skainet_player_pause(player);
    printf("Pause\n");
    return 0;
}

static int cmd_stop(int argc, char **argv)
{
    if (player == NULL) {
        printf("Player not created\n");
        return 0;
    }
    esp_skainet_player_pause(player);
    printf("Stop\n");
    return 0;
}

static int cmd_vol_up(int argc, char **argv)
{
    if (player == NULL) {
        printf("Player not created\n");
        return 0;
    }
    esp_skainet_player_increase_vol(player);
    int vol = 0;
    esp_audio_get_play_vol(&vol);
    printf("Volume: %d\n", vol);
    return 0;
}

static int cmd_vol_down(int argc, char **argv)
{
    if (player == NULL) {
        printf("Player not created\n");
        return 0;
    }
    esp_skainet_player_decrease_vol(player);
    int vol = 0;
    esp_audio_get_play_vol(&vol);
    printf("Volume: %d\n", vol);
    return 0;
}

static void register_player_commands(void)
{
    const esp_console_cmd_t play_cmd = {
        .command = "play",
        .help = "Resume playback",
        .hint = NULL,
        .func = &cmd_play,
    };
    ESP_ERROR_CHECK(esp_console_cmd_register(&play_cmd));

    const esp_console_cmd_t pause_cmd = {
        .command = "pause",
        .help = "Pause playback",
        .hint = NULL,
        .func = &cmd_pause,
    };
    ESP_ERROR_CHECK(esp_console_cmd_register(&pause_cmd));

    const esp_console_cmd_t stop_cmd = {
        .command = "stop",
        .help = "Stop playback",
        .hint = NULL,
        .func = &cmd_stop,
    };
    ESP_ERROR_CHECK(esp_console_cmd_register(&stop_cmd));

    const esp_console_cmd_t vol_up_cmd = {
        .command = "vol+",
        .help = "Volume up",
        .hint = NULL,
        .func = &cmd_vol_up,
    };
    ESP_ERROR_CHECK(esp_console_cmd_register(&vol_up_cmd));

    const esp_console_cmd_t vol_down_cmd = {
        .command = "vol-",
        .help = "Volume down",
        .hint = NULL,
        .func = &cmd_vol_down,
    };
    ESP_ERROR_CHECK(esp_console_cmd_register(&vol_down_cmd));
}

void app_main(void)
{
    ESP_ERROR_CHECK(esp_board_init(16000, 1, 16));
    ESP_ERROR_CHECK(esp_sdcard_init("/sdcard", 10));

    player = esp_skainet_player_create(RINGBUF_SIZE, 1, 2, 32, 16000);
    if (player == NULL) {
        printf("Failed to create player\n");
        return;
    }

    int vol = 0;
    esp_audio_set_play_vol(78);
    esp_audio_get_play_vol(&vol);
    printf("Player volume: %d\n", vol);

    printf("Playing WAV files from %s (use play/pause/stop/vol+/vol-)\n", MUSIC_PATH);
    esp_skainet_player_play(player, MUSIC_PATH);
    esp_skainet_player_pause(player);

    /* Console REPL for playback control */
    esp_console_repl_t *repl = NULL;
    esp_console_repl_config_t repl_config = ESP_CONSOLE_REPL_CONFIG_DEFAULT();
    repl_config.prompt = "player>";
    esp_console_dev_uart_config_t uart_config = ESP_CONSOLE_DEV_UART_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_console_new_repl_uart(&uart_config, &repl_config, &repl));
    esp_console_register_help_command();
    register_player_commands();
    ESP_ERROR_CHECK(esp_console_start_repl(repl));
}
