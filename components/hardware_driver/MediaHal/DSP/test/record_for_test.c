
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "driver/i2s.h"
#include "string.h"
#include "esp_log.h"
#include "board.h"
#include "EspAudioAlloc.h"
#include "FatFsSal.h"

static char *TAG = "REC_TEST";

// int I2s1_init(void)
// {
//     int res = 0;
//     i2s_config_t i2s_config = {
//         .mode = I2S_MODE_MASTER | I2S_MODE_RX,//the mode must be set according to DSP configuration
//         .sample_rate = 16000,                           //must be the same as DSP configuration
//         .channel_format = I2S_CHANNEL_FMT_RIGHT_LEFT,   //must be the same as DSP configuration
//         .bits_per_sample = 16,                          //must be the same as DSP configuration
//         .communication_format = I2S_COMM_FORMAT_I2S,
//         //when dma_buf_count = 3 and dma_buf_len = 300, then 3 * 4 * 300 * 2 Bytes internal RAM will be used. The multiplier 2 is for Rx buffer and Tx buffer together.
//         .dma_buf_count = 3,                            /*!< amount of the dam buffer sectors*/
//         .dma_buf_len = 300,                            /*!< dam buffer size of each sector (word, i.e. 4 Bytes) */
//         .intr_alloc_flags = ESP_INTR_FLAG_LEVEL2,
// //        .use_apll = 1,//pdm can not use apll
// //        .fixed_mclk = 12288000,//must be the same as 12.288MHZ according to DSP datasheet
//     };
//     i2s_pin_config_t pin_config = {
//         .bck_io_num = DSP_RECV_I2S_BCK,
//         .ws_io_num = DSP_RECV_I2S_LRCK,
//         .data_out_num = -1,
//         .data_in_num = DSP_RECV_I2S_DATA
//     };
//     res = i2s_driver_install(1, &i2s_config, 0, NULL);
//     if (res)
//         return res;
//     res |= i2s_set_pin(1, &pin_config);
//     i2s_zero_dma_buffer(1);

//     return res;
// }
#define PLAYBACK_EN 0

void putTask(void *pv)
{
    // I2s1_init();
    vTaskDelay(5000 / portTICK_RATE_MS);
#define OUTBUF_SIZE (5 * 1024)
#if PLAYBACK_EN == 0
    FILE *file = fopen("/sdcard/DspPdmMono.pcm", "w+");//pdm is always mono
    if (NULL == file) {
        ESP_LOGE(TAG, "open file failed,[%d]", __LINE__);
        vTaskDelete(NULL);
        return;
    }
#endif
    uint8_t *outBuf = (uint8_t *)EspAudioAllocInner(1, OUTBUF_SIZE);
    if (NULL == outBuf) {
#if PLAYBACK_EN == 0
        fclose(file);
#endif
        ESP_LOGE(TAG, "outBuf malloc failed[%d]", __LINE__);
        vTaskDelete(NULL);
        return;
    }
    while (1) {
        // ESP_LOGI(TAG, "put running,%d", OUTBUF_SIZE);
        int ret = i2s_read_bytes(1, (char *)outBuf, OUTBUF_SIZE, portMAX_DELAY);
#if PLAYBACK_EN == 1
        i2s_write_bytes(0, (char *)outBuf, OUTBUF_SIZE, portMAX_DELAY);
#else
        FatfsComboWrite(outBuf, 1, OUTBUF_SIZE, file);
#endif
    }
    free(outBuf);
#if PLAYBACK_EN == 0
    fclose(file);
#endif
    vTaskDelete(NULL);
}

void recorder_create()
{
    xTaskCreatePinnedToCore(&putTask, "putTask", 3 * 1024, NULL, 1, NULL, 1);
}
