#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/fcntl.h>
#include <sys/errno.h>
#include <sys/unistd.h>
#include <sys/select.h>
#include <sys/time.h>
#include "string.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "sdkconfig.h"
#include "driver/i2s.h"
#include "esp_log.h"
#include "InterruptionSal.h"
#include "userconfig.h"
#include "MediaHal.h"
#include "ES8388_interface.h"
#include "ES8374_interface.h"
#include "es8311.h"
#include <sys/time.h>
#include "esp_tts.h"
#include "esp_tts_voice_xiaole.h"
#include "esp_tts_player.h"
#include "esp_log.h"
#include "ringbuf.h"
#include "esp_vfs.h"
#include "esp_vfs_dev.h"
#include "driver/uart.h"

#define TAG "ESP_TTS_zh_CN"

struct RingBuf *urat_rb;

void tts_codec_init(void)
{
    int ret = 0;
#if (defined CONFIG_CODEC_CHIP_IS_ES8388)
    Es8388Config  Es8388Conf =  AUDIO_CODEC_ES8388_DEFAULT();
    ret = MediaHalInit(&Es8388Conf);
    if (ret) {
        ESP_LOGE(TAG, "MediaHal init failed, line:%d", __LINE__);
    }
    ESP_LOGI(TAG, "CONFIG_CODEC_CHIP_IS_ES8388");
#elif (defined CONFIG_CODEC_CHIP_IS_ES8374)
    Es8374Config  Es8374Conf =  AUDIO_CODEC_ES8374_DEFAULT();
    ret = MediaHalInit(&Es8374Conf);
    if (ret) {
        ESP_LOGI(TAG, "MediaHal init failed, line:%d", __LINE__);
    }
    ESP_LOGI(TAG, "CONFIG_CODEC_CHIP_IS_ES8374");

#elif (defined CONFIG_CODEC_CHIP_IS_ES8311)
    Es8311Config  es8311Cfg =  AUDIO_CODEC_ES8311_DEFAULT();
    ret = MediaHalInit(&es8311Cfg);
    if (ret) {
        ESP_LOGI(TAG, "MediaHal init failed, line:%d", __LINE__);
    }
    ESP_LOGI(TAG, "CONFIG_CODEC_CHIP_IS_ES8311");

#endif
    MediaHalSetVolume(45);
}

int iot_dac_audio_play(const uint8_t* data, int length, TickType_t ticks_to_wait)
{
    size_t bytes_write = 0;
    i2s_write(0, (const char*) data, length, &bytes_write, ticks_to_wait);
    return ESP_OK;
}



/* From WmfDecBytesPerFrame in dec_input_format_tab.cpp */
const int sizes[] = { 12, 13, 15, 17, 19, 20, 26, 31, 5, 6, 5, 5, 0, 0, 0, 0 };
#define URAT_BUF_LEN 1024

void uartTask(void *arg)
{
    uart_config_t uart_config = {
        .baud_rate = 115200,
        .data_bits = UART_DATA_8_BITS,
        .parity    = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE
    };
    uart_param_config(UART_NUM_0, &uart_config);
    uart_driver_install(UART_NUM_0, 2*1024, 0, 0, NULL, 0);
    char data[1024];
    int len=0;
    char *prompt="请输入任意短语:";
    printf("%s\n", prompt);

    while (1) {
        int fd;

        if ((fd = open("/dev/uart/0", O_RDWR)) == -1) {
            ESP_LOGE(TAG, "Cannot open UART");
            vTaskDelay(5000 / portTICK_PERIOD_MS);
            continue;
        }

        // We have a driver now installed so set up the read/write functions to use driver also.
        esp_vfs_dev_uart_use_driver(0);

        while (1) {
            int s;
            fd_set rfds;
            struct timeval tv = {
                .tv_sec = 5,
                .tv_usec = 0,
            };

            FD_ZERO(&rfds);
            FD_SET(fd, &rfds);

            s = select(fd + 1, &rfds, NULL, NULL, &tv);

            if (s < 0) {
                ESP_LOGE(TAG, "Select failed: errno %d", errno);
                break;
            } else if (s == 0) {
                continue;
            } else {
                if (FD_ISSET(fd, &rfds)) {
                    char buf;
                    if (read(fd, &buf, 1) > 0) {
                        ESP_LOGI("", "%c", buf);
                        rb_write(urat_rb, (uint8_t *)&buf, 1, portMAX_DELAY);

                        if (buf=='\n') {
                            data[len]='\0';
                            printf("uart input: %s\n", data);
                            len=0;
                        } else {
                            data[len]=buf;
                            len++;
                        }
                        // Note: Only one character was read even the buffer contains more. The other characters will
                        // be read one-by-one by subsequent calls to select() which will then return immediately
                        // without timeout.
                    } else {
                        ESP_LOGE(TAG, "UART read error");
                        break;
                    }
                } else {
                    ESP_LOGE(TAG, "No FD has been set in select()");
                    break;
                }
            }
        }

        close(fd);
    }

    vTaskDelete(NULL);
}

int app_main() {
    tts_codec_init();       
    printf("RAM size: %dKB\n", heap_caps_get_free_size(MALLOC_CAP_8BIT) / 1024);
    esp_tts_voice_t *voice=&esp_tts_voice_xiaole;
    esp_tts_handle_t *tts_handle=esp_tts_create(voice);
    urat_rb = rb_init(BUFFER_PROCESS+1, URAT_BUF_LEN, 1, NULL);
    char data[URAT_BUF_LEN+1];
    char buf;
    int data_len=0;

    char *prompt="欢迎使用乐鑫语音合成";  
    if (esp_tts_parse_chinese(tts_handle, prompt)) {
            int len[1]={0};
            do {
                short *data=esp_tts_stream_play(tts_handle, len, 4);
                iot_dac_audio_play(data, len[0]*2, portMAX_DELAY);
                //printf("data:%d \n", len[0]);
            } while(len[0]>0);
            i2s_zero_dma_buffer(0);
    }
    xTaskCreatePinnedToCore(&uartTask, "urat", 3 * 1024, NULL, 5, NULL, 0);

    while (1) {
        rb_read(urat_rb, (uint8_t *)&buf, 1, portMAX_DELAY);

        if(buf=='\n') {
            // start to run tts
            data[data_len]='\0'; 
            printf("tts get:%s\n", data);
            if (esp_tts_parse_chinese(tts_handle, data)) {
                int len[1]={0};
                do {
                    short *data=esp_tts_stream_play(tts_handle, len, 4);
                    iot_dac_audio_play(data, len[0]*2, portMAX_DELAY);
                } while(len[0]>0);
                i2s_zero_dma_buffer(0);
            }

            data_len=0;
        } else if(data_len<URAT_BUF_LEN) {
            // append urat buffer into data
            data[data_len]=buf; 
            data_len++;
        } else {
            printf("ERROR: out of range\n");
            data_len=0;
        }
    }
}

