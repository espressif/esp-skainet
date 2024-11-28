#include "mn_perf_tester.h"
#include "model_path.h"
#include "esp_board_init.h"
#include "esp_process_sdkconfig.h"

static int start_vad_test(int argc, char **argv)
{
    printf("Start to test VAD:\n");

    srmodel_list_t *models = esp_srmodel_init("model");
    char *model_name = esp_srmodel_filter(models, "vadnet", NULL);
    char csv_file[128];
    char log_file[128];
    if (model_name == NULL) {
        sprintf(csv_file, "/sdcard/%s.csv", "webrtc_vad");
        sprintf(log_file, "/sdcard/%s.log", "webrtc_vad");
    } else {
        sprintf(csv_file, "/sdcard/%s.csv", "webrtc_vad");
        sprintf(log_file, "/sdcard/%s.log", model_name);
    }
    printf("test:%s, log:%s\n", csv_file, log_file);

    // Select speech enhancement pipeline
    afe_config_t afe_config = AFE_CONFIG_DEFAULT();
    afe_config.aec_init = false;
    afe_config.se_init = false;
    afe_config.pcm_config.mic_num = 2;
    afe_config.pcm_config.ref_num = 1;
    afe_config.pcm_config.total_ch_num = 3;
    afe_config.vad_model_name = model_name;
    afe_config.vad_mode = VAD_MODE_3;
    afe_config.vad_min_speech_ms = 32;
    afe_config.vad_min_noise_ms = 128;

    afe_config.wakenet_model_name = esp_srmodel_filter(models, ESP_WN_PREFIX, NULL);;
    afe_config.wakenet_mode = DET_MODE_3CH_90;
    perf_tester_config_t *tester_config = get_perf_tester_config();
    offline_vad_tester(csv_file, log_file, &ESP_AFE_SR_HANDLE, &afe_config,
                       TESTER_WAV_3CH, tester_config);
    return 0;
}

void app_main()
{
    ESP_ERROR_CHECK(esp_board_init(16000, 1, 16));
    ESP_ERROR_CHECK(esp_sdcard_init("/sdcard", 10));
    heap_caps_print_heap_info(MALLOC_CAP_8BIT);

    // Init console repl
    esp_console_repl_t *repl = NULL;
    esp_console_repl_config_t repl_config = ESP_CONSOLE_REPL_CONFIG_DEFAULT();
    repl_config.prompt = "perf_tester>";
    esp_console_dev_uart_config_t uart_config = ESP_CONSOLE_DEV_UART_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_console_new_repl_uart(&uart_config, &repl_config, &repl));

    // Register cmd
    esp_console_register_help_command();
    register_perf_tester_config_cmd();
    register_perf_tester_start_cmd(&start_vad_test);

    // Start console repl
    ESP_ERROR_CHECK(esp_console_start_repl(repl));
}