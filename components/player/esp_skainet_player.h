#ifndef _ESP_SKAINET_PLAYER_
#define _ESP_SKAINET_PLAYER_


typedef void* player_handle;
#define FATFS_PATH_LENGTH_MAX 256

void *esp_skainet_player_create(int ringbuf_size, unsigned int core_num);
void esp_skainet_player_play(void *handle, const char *path);
void esp_skainet_player_pause(void *handle);
void esp_skainet_player_continue(void *handle);
void esp_skainet_player_exit(void *handle);
int esp_skainet_player_get_state(void *handle);
void esp_skainet_player_increase_vol(void *handle);
void esp_skainet_player_decrease_vol(void *handle);

#endif
