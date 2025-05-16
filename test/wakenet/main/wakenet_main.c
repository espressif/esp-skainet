#include "wn_perf_tester.h"
#include "esp_afe_config.h"
#include "model_path.h"
#include "esp_board_init.h"
#include "esp_mn_speech_commands.h"
#include "esp_process_sdkconfig.h"
#include "perf_tester_cmd.h"
#include "esp_log.h"
#include "esp_sr_debug.h"

static void* wakenet_test(srmodel_list_t *models, const char *csv_file, const char* log_file)
{
    // Select speech enhancement pipeline
    afe_config_t *afe_config = afe_config_init("MMR", models, AFE_TYPE_SR, AFE_MODE_LOW_COST);
    afe_config->ns_init = false;
    afe_config->ns_model_name = NULL;
    perf_tester_config_t *tester_config = get_perf_tester_config();
    afe_config_print(afe_config);
    void *task_handle = offline_wn_tester_start(csv_file, log_file, NULL, afe_config, TESTER_WAV_3CH, tester_config);
    afe_config_free(afe_config);
    return task_handle;
}

static int start_rar_test(int argc, char **argv)
{
    printf("Start RAR test:\n");
    srmodel_list_t *models = esp_srmodel_init("model");
    char *wn_name = esp_srmodel_filter(models, ESP_WN_PREFIX, NULL);
    char csv_file[128];
    char log_file[128];
    sprintf(csv_file, "/sdcard/%s.csv", wn_name);
    sprintf(log_file, "/sdcard/%s.log", wn_name);
    printf("test:%s, log:%s\n", csv_file, log_file);

    esp_sr_set_debug_mode(1);
    wakenet_test(models, csv_file, log_file);

    return 0;
}

static int start_far_test(int argc, char **argv)
{
    printf("Start FAR test (48-hour dataset):\n");
    srmodel_list_t *models = esp_srmodel_init("model");
    char *wn_name = esp_srmodel_filter(models, ESP_WN_PREFIX, NULL);
    char csv_file[128] = "/sdcard/far_48h.csv";
    char log_file[128];
    sprintf(log_file, "/sdcard/%s_far.log", wn_name);
    printf("test:%s, log:%s\n", csv_file, log_file);

    esp_sr_set_debug_mode(1);
    wakenet_test(models, csv_file, log_file);
    return 0;
}

void register_test_commands()
{
    // Register RAR command
    const esp_console_cmd_t rar_cmd = {
        .command = "rar",
        .help = "Run RAR test, please make sure the <model_name>.csv is in /sdcard",
        .hint = NULL,
        .func = start_rar_test,
    };
    ESP_ERROR_CHECK(esp_console_cmd_register(&rar_cmd));

    // Register FAR command
    const esp_console_cmd_t far_cmd = {
        .command = "far",
        .help = "Run FAR test (48-hour dataset), please make sure the far_48hours.csv is in /sdcard",
        .hint = NULL,
        .func = start_far_test,
    };
    ESP_ERROR_CHECK(esp_console_cmd_register(&far_cmd));
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

    // Register commands
    esp_console_register_help_command();
    register_perf_tester_config_cmd();
    register_test_commands();

    // Start console repl
    ESP_ERROR_CHECK(esp_console_start_repl(repl));
}
