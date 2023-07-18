#pragma once
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "esp_afe_sr_iface.h"
#include "esp_afe_sr_models.h"
#include "esp_wn_iface.h"
#include "esp_wn_models.h"
#include "perf_tester_cmd.h"

typedef enum {
    TESTER_PCM_3CH = 0,        // input 3 channel PCM [mic1, mic2, ref, ...]
    TESTER_WAV_3CH = 1,        // input 3 channel WAV [mic1, mic2, ref, ...]
    TESTER_PCM_1CH = 2,        // input 1 channel PCM
    TESTER_WAV_1CH = 3,        // input 1 channel WAV
} tester_audio_t;


/**
 * @brief Test wakenet and AFE pipeline performance,include CPU loading, memory size and trigger times
 *
 * @param csv_file       CSV file path
 * @param log_file       File name to save test report
 * @param afe_handle     Handle of speech front end
 * @param afe_config     Config of afe handle
 * @param audio_type     Input audio type, 0: 3channel-PCM; 1: 3channel-WAV
 * @param tester_config  Perf tester config
 */
void offline_wn_tester(const char *csv_file,
                       const char *log_file,
                       const esp_afe_sr_iface_t *afe_handle,
                       afe_config_t *afe_config,
                       int audio_type,
                       perf_tester_config_t *config);