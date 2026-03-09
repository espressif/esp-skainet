#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_tts.h"
#include "esp_tts_voice_xiaole.h"
#include "esp_tts_voice_template.h"
#include "esp_tts_player.h"
#include "esp_board_init.h"
#include "ringbuf.h"

#include "tts_urat.h"

#include "wav_encoder.h"
#include "esp_partition.h"
#include "esp_idf_version.h"

//#define SDCARD_OUTPUT_ENABLE

ringbuf_handle_t urat_rb=NULL;

int app_main()
{
#if defined CONFIG_ESP32_S3_EYE_BOARD
    printf("Not Support esp32-s3-eye board\n");
    return 0;
#endif

    ESP_ERROR_CHECK(esp_board_init(16000, 1, 16));
#ifdef SDCARD_OUTPUT_ENABLE
    ESP_ERROR_CHECK(esp_sdcard_init("/sdcard", 10));
    FILE* fp=fopen("/sdcard/URAT.pcm", "w+");
    if(fp==NULL)
        printf("can not open file!\n");

    //sample rate:16000Hz, int16, mono
    void * wav_encoder=wav_encoder_open("/sdcard/prompt.wav", 16000, 16, 1);
    void * urat_wav_encoder=wav_encoder_open("/sdcard/URAT.wav", 16000, 16, 1);

#endif

    /*** 1. create esp tts handle ***/
    // initial voice set from separate voice data partition

    const esp_partition_t* part=esp_partition_find_first(ESP_PARTITION_TYPE_DATA, ESP_PARTITION_SUBTYPE_ANY, "voice_data");
    if (part==NULL) { 
        printf("Couldn't find voice data partition!\n"); 
        return 0;
    } else {
        printf("voice_data paration size:%d\n", part->size);
    }
    void* voicedata;
#if ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(5, 0, 0)
    esp_partition_mmap_handle_t mmap;
    esp_err_t err=esp_partition_mmap(part, 0, part->size, ESP_PARTITION_MMAP_DATA, &voicedata, &mmap);
#else
    spi_flash_mmap_handle_t mmap;
    esp_err_t err=esp_partition_mmap(part, 0, part->size, SPI_FLASH_MMAP_DATA, &voicedata, &mmap);
#endif
    if (err != ESP_OK) {
        printf("Couldn't map voice data partition!\n"); 
        return 0;
    }
    esp_tts_voice_t *voice=esp_tts_voice_set_init(&esp_tts_voice_template, (int16_t*)voicedata); 
    

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
                esp_audio_play(pcm_data, len[0]*2, portMAX_DELAY);
#endif
                //printf("data:%d \n", len[0]);
            } while(len[0]>0);
    }
    esp_tts_stream_reset(tts_handle);
#ifdef SDCARD_OUTPUT_ENABLE
    wav_encoder_close(wav_encoder);
#endif

    /*** 3. play urat input text ***/
    urat_rb = rb_create(URAT_BUF_LEN, 1);  // urat ringbuf init
    char data[URAT_BUF_LEN+1];
    char in;
    int data_len=0;

    xTaskCreatePinnedToCore(&uartTask, "urat", 6 * 1024, NULL, 5, NULL, 0);
    char *prompt2="\n请输入短语:";
    printf("%s\n", prompt2);
 
    while (1) {
        rb_read(urat_rb, &in, 1, portMAX_DELAY);

        if(in=='\n') {
            // start to run tts
            data[data_len]='\0'; 
            printf("tts input:%s\n", data);
            if (esp_tts_parse_chinese(tts_handle, data)) {
                int len[1]={0};
                do {
                    short *pcm_data=esp_tts_stream_play(tts_handle, len, 3);
#ifdef SDCARD_OUTPUT_ENABLE
                    wav_encoder_run(urat_wav_encoder, pcm_data, len[0]*2);
#else
                    esp_audio_play(pcm_data, len[0]*2, portMAX_DELAY);
#endif
                } while(len[0]>0);
                // i2s_zero_dma_buffer(0);
            }
            esp_tts_stream_reset(tts_handle);
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
#ifdef SDCARD_OUTPUT_ENABLE
    wav_encoder_close(urat_wav_encoder);
#endif

    return 0;
}

