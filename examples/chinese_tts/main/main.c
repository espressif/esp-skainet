#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "driver/i2s.h"
#include "MediaHal.h"
#include "esp_tts.h"
#include "esp_tts_voice_xiaole.h"
#include "esp_tts_player.h"
#include "ringbuf.h"

#include "tts_urat.h"

struct RingBuf *urat_rb=NULL;

// i2s play
int iot_dac_audio_play(const uint8_t* data, int length, TickType_t ticks_to_wait)
{
    size_t bytes_write = 0;
    i2s_write(0, (const char*) data, length, &bytes_write, ticks_to_wait);
    return ESP_OK;
}

int app_main() {
    codec_init();      

    // use pre-define xiaole voice to create tts handle
    esp_tts_voice_t *voice=&esp_tts_voice_xiaole;
    esp_tts_handle_t *tts_handle=esp_tts_create(voice);

    // urat ringbuf init
    urat_rb = rb_init(BUFFER_PROCESS+1, URAT_BUF_LEN, 1, NULL);
    char data[URAT_BUF_LEN+1];
    char in;
    int data_len=0;

    // play prompt text
    char *prompt1="欢迎使用乐鑫语音合成";  
    printf("%s\n", prompt1);
    if (esp_tts_parse_chinese(tts_handle, prompt1)) {
            int len[1]={0};
            do {
                short *data=esp_tts_stream_play(tts_handle, len, 4);
                iot_dac_audio_play(data, len[0]*2, portMAX_DELAY);
                //printf("data:%d \n", len[0]);
            } while(len[0]>0);
            i2s_zero_dma_buffer(0);
    }

    xTaskCreatePinnedToCore(&uartTask, "urat", 3 * 1024, NULL, 5, NULL, 0);
    char *prompt2="\n请输入短语:";
    printf("%s\n", prompt2);

    // play urat input text
    while (1) {
        rb_read(urat_rb, (uint8_t *)&in, 1, portMAX_DELAY);

        if(in=='\n') {
            // start to run tts
            data[data_len]='\0'; 
            printf("tts input:%s\n", data);
            if (esp_tts_parse_chinese(tts_handle, data)) {
                int len[1]={0};
                do {
                    short *data=esp_tts_stream_play(tts_handle, len, 4);
                    iot_dac_audio_play(data, len[0]*2, portMAX_DELAY);
                } while(len[0]>0);
                i2s_zero_dma_buffer(0);
            }
            printf("%s\n", prompt2);
            data_len=0;
        } else if(data_len<URAT_BUF_LEN) {
            // append urat buffer into data
            data[data_len]=in; 
            data_len++;
        } else {
            printf("ERROR: out of range\n");
            data_len=0;
        }
    }

}

