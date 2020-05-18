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
#include "esp_tts_voice_template.h"
#include "esp_tts_player.h"
#include "ringbuf.h"

#include "tts_urat.h"
#include "sdcard_init.h"

#include "wav_encoder.h"
#include "esp_partition.h"

//#define SDCARD_OUTPUT_ENABLE

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

#ifdef SDCARD_OUTPUT_ENABLE
    sd_card_mount("/sdcard");
    FILE* fp=fopen("/sdcard/URAT.pcm", "w+");
    if(fp==NULL)
        printf("can not open file!\n");

    //sample rate:16000Hz, int16, mono
    void * wav_encoder=wav_encoder_open("/sdcard/prompt.wav", 16000, 16, 1);
#endif

    /*** 1. create esp tts handle ***/
    // method1: use pre-define xiaole voice lib.
    // This method is not recommended because the method may make app bin exceed the limit of esp32  
    // esp_tts_voice_t *voice=&esp_tts_voice_xiaole;

    // method2: initial voice set from separate voice data partition
    const esp_partition_t* part=esp_partition_find_first(ESP_PARTITION_TYPE_DATA, ESP_PARTITION_SUBTYPE_DATA_FAT, "voice_data");
    if (part==0) { 
        printf("Couldn't find voice data partition!\n"); 
        return 0;
    }
    spi_flash_mmap_handle_t mmap;
    uint16_t* voicedata;
    esp_err_t err=esp_partition_mmap(part, 0, 3*1024*1024, SPI_FLASH_MMAP_DATA, (const void**)&voicedata, &mmap);
    if (err != ESP_OK) {
        printf("Couldn't map voice data partition!\n"); 
        return 0;
    }
    esp_tts_voice_t *voice=esp_tts_voice_set_init(&esp_tts_voice_template, voicedata); 

    esp_tts_handle_t *tts_handle=esp_tts_create(voice);

    /*** 2. play prompt text ***/
    char *prompt1="欢迎使用乐鑫语音合成";  
    printf("%s\n", prompt1);
    if (esp_tts_parse_chinese(tts_handle, prompt1)) {
            int len[1]={0};
            do {
                short *pcm_data=esp_tts_stream_play(tts_handle, len, 3);
#ifdef SDCARD_OUTPUT_ENABLE
                wav_encoder_run(wav_encoder, pcm_data, len[0]*2);
#else
                iot_dac_audio_play(pcm_data, len[0]*2, portMAX_DELAY);
#endif
                //printf("data:%d \n", len[0]);
            } while(len[0]>0);
            i2s_zero_dma_buffer(0);
    }
#ifdef SDCARD_OUTPUT_ENABLE
    wav_encoder_close(wav_encoder);
#endif

    /*** 3. play urat input text ***/
    urat_rb = rb_init(BUFFER_PROCESS+1, URAT_BUF_LEN, 1, NULL);  // urat ringbuf init
    char data[URAT_BUF_LEN+1];
    char in;
    int data_len=0;

    xTaskCreatePinnedToCore(&uartTask, "urat", 3 * 1024, NULL, 5, NULL, 0);
    char *prompt2="\n请输入短语:";
    printf("%s\n", prompt2);
 
    while (1) {
        rb_read(urat_rb, (uint8_t *)&in, 1, portMAX_DELAY);

        if(in=='\n') {
            // start to run tts
            data[data_len]='\0'; 
            printf("tts input:%s\n", data);
            if (esp_tts_parse_chinese(tts_handle, data)) {
                int len[1]={0};
                do {
                    short *pcm_data=esp_tts_stream_play(tts_handle, len, 3);
#ifdef SDCARD_OUTPUT_ENABLE
                    FatfsComboWrite(pcm_data, 1, len[0]*2, fp);
#else
                    iot_dac_audio_play(pcm_data, len[0]*2, portMAX_DELAY);
#endif
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

    return 0;
}

