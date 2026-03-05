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

#include "esp_wn_iface.h"
#include "esp_wn_models.h"
#include "esp_afe_sr_models.h"
#include "esp_mn_iface.h"
#include "esp_mn_models.h"
#include "model_path.h"
#include "string.h"

#define MUSIC_PATH    "/sdcard/music/"
#define RINGBUF_SIZE  (4096 * 4)

static void *player = NULL;
int detect_flag = 0;
static esp_afe_sr_iface_t *afe_handle = NULL;
static volatile int task_flag = 1;
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


void feed_Task(void *arg)
{
    esp_afe_sr_data_t *afe_data = arg;
    int audio_chunksize = afe_handle->get_feed_chunksize(afe_data);
    int nch = afe_handle->get_feed_channel_num(afe_data);
    int feed_channel = esp_get_feed_channel();
    assert(nch==feed_channel);
    int16_t *i2s_buff = malloc(audio_chunksize * sizeof(int16_t) * feed_channel);
    assert(i2s_buff);

    while (task_flag) {
        esp_get_feed_data(true, i2s_buff, audio_chunksize * sizeof(int16_t) * feed_channel);

        afe_handle->feed(afe_data, i2s_buff);
    }
    if (i2s_buff) {
        free(i2s_buff);
        i2s_buff = NULL;
    }
    vTaskDelete(NULL);
}

void detect_Task(void *arg)
{
    esp_afe_sr_data_t *afe_data = arg;
    int afe_chunksize = afe_handle->get_fetch_chunksize(afe_data);
    int16_t *buff = malloc(afe_chunksize * sizeof(int16_t));
    assert(buff);
    printf("------------LISTENING------------\n");

    while (task_flag) {
        afe_fetch_result_t* res = afe_handle->fetch(afe_data); 
        if (!res || res->ret_value == ESP_FAIL) {
            printf("fetch error!\n");
            break;
        }
        // printf("vad state: %d\n", res->vad_state);

        if (res->wakeup_state == WAKENET_DETECTED) {
            printf("\nDETECTED\n");
        }
    }
    if (buff) {
        free(buff);
        buff = NULL;
    }
    vTaskDelete(NULL);
}


void app_main(void)
{
    ESP_ERROR_CHECK(esp_board_init(16000, 1, 16));
    ESP_ERROR_CHECK(esp_sdcard_init("/sdcard", 10));

    srmodel_list_t *models = esp_srmodel_init("model");
    if (models) {
        for (int i=0; i<models->num; i++) {
            if (strstr(models->model_name[i], ESP_WN_PREFIX) != NULL) {
                printf("wakenet model in flash: %s\n", models->model_name[i]);
            }
        }
    }

    afe_config_t *afe_config = afe_config_init(esp_get_input_format(), models, AFE_TYPE_SR, AFE_MODE_LOW_COST);
    afe_handle = esp_afe_handle_from_config(afe_config);
    afe_config->vad_init=false;

    esp_afe_sr_data_t *afe_data = afe_handle->create_from_config(afe_config);
    afe_config_free(afe_config);
    player = esp_skainet_player_create(RINGBUF_SIZE, 0, 2, 32, 16000);
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
    xTaskCreatePinnedToCore(&feed_Task, "feed", 8 * 1024, (void*)afe_data, 5, NULL, 0);
    xTaskCreatePinnedToCore(&detect_Task, "detect", 4 * 1024, (void*)afe_data, 5, NULL, 1);

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
