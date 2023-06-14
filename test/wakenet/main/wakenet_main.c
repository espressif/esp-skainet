#include "perf_tester.h"
#include "model_path.h"
#include "esp_board_init.h"
#include "esp_mn_speech_commands.h"
#include "esp_process_sdkconfig.h"

void app_main()
{
    ESP_ERROR_CHECK(esp_board_init(AUDIO_HAL_16K_SAMPLES, 1, 16));
    ESP_ERROR_CHECK(esp_sdcard_init("/sdcard", 10));
    heap_caps_print_heap_info(MALLOC_CAP_8BIT);

    srmodel_list_t *models = esp_srmodel_init("model");
    char *wn_name = esp_srmodel_filter(models, ESP_WN_PREFIX, NULL);
    char csv_file[128];
    char log_file[128];
    sprintf(csv_file, "/sdcard/%s.csv", wn_name);
    sprintf(log_file, "/sdcard/%s.log", wn_name);
    printf("test:%s, log:%s\n", csv_file, log_file);

    // Select speech enhancement pipeline
    afe_config_t afe_config = AFE_CONFIG_DEFAULT();
    afe_config.wakenet_model_name = wn_name;
    afe_config.wakenet_mode = DET_MODE_3CH_90;

    offline_wn_tester(csv_file, log_file, &ESP_AFE_SR_HANDLE, &afe_config, TESTER_WAV_3CH);
}