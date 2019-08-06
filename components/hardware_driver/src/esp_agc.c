/* esp_agc.c */

#include "esp_agc.h"
#include "gain_control.h"
#include "analog_agc.h"

void *esp_agc_open(int agc_mode, int sample_rate)
{
    if (sample_rate != 8000 && sample_rate != 16000 && sample_rate != 32000) {
        return NULL;
    }

    void *agcHandle = NULL;
    WebRtcAgc_Create(&agcHandle);

    int minLevel = 0;
    int maxLevel = 255;

    WebRtcAgc_Init(agcHandle, minLevel, maxLevel, agc_mode, sample_rate);

    return agcHandle;
}

void set_agc_config(void *agc_handle, int gain_dB, int limiter_enable, int target_level_dbfs)
{
    WebRtcAgc_config_t agcConfig;
    agcConfig.compressionGaindB = gain_dB;
    agcConfig.limiterEnable = limiter_enable;
    agcConfig.targetLevelDbfs = target_level_dbfs;
    WebRtcAgc_set_config(agc_handle, agcConfig);
}

#define FRAME_IN_MILLISECONDS (10)
int esp_agc_process(void *agc_handle, short *in_pcm, short *out_pcm, int frame_size, int sample_rate)
{
    int inMicLevel = 0;
    int outMicLevel = 0;
    unsigned char saturationWarning;

    Agc_t *stt = (Agc_t *)agc_handle;

    if (sample_rate != stt->fs) {
        return ESP_AGC_SAMPLE_RATE_ERROR;
    }

    int frame_millseconds = 1000 * frame_size / sample_rate;
    if (frame_millseconds != FRAME_IN_MILLISECONDS) {
        return ESP_AGC_FRAME_SIZE_ERROR;
    }

    int nAgcRet = WebRtcAgc_Process(agc_handle, in_pcm, NULL, frame_size, out_pcm, NULL, inMicLevel, &outMicLevel, 0, &saturationWarning);
    if (nAgcRet != 0) {
        return ESP_AGC_FAIL;
    }

    return ESP_AGC_SUCCESS;
}

void esp_agc_clse(void *agc_handle)
{
    if (agc_handle) {
        WebRtcAgc_Free(agc_handle);
    }
}