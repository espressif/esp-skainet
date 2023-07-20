#include "stdio.h"
#include "esp_log.h"
#include "assert.h"
#include "string.h"
#include "perf_tester_cmd.h"

// static const char *TAG = "perf_tester_cmd";
static struct {
    struct arg_str *mode;
    struct arg_str *noise;
    struct arg_str *snr;
    struct arg_end *end;
} teser_config_args;

static perf_tester_config_t *config = NULL;


static int run_tester_config(int argc, char **argv)
{
    if (config == NULL) {
        config = (perf_tester_config_t*) malloc(sizeof(perf_tester_config_t));
    }

    int nerrors = arg_parse(argc, argv, (void **) &teser_config_args);
    if (nerrors != 0) {
        arg_print_errors(stderr, teser_config_args.end, argv[0]);
        return 1;
    }

    assert(teser_config_args.mode->count == 1);
    assert(teser_config_args.noise->count == 1);
    assert(teser_config_args.snr->count == 1);
    strcpy(config->mode, teser_config_args.mode->sval[0]);
    strcpy(config->noise, teser_config_args.noise->sval[0]);
    strcpy(config->snr, teser_config_args.snr->sval[0]);
    printf("mode:%s, noise:%s, snr:%s\n", config->mode, config->noise, config->snr);
    return 0;
}

void register_perf_tester_config_cmd(void)
{
    teser_config_args.mode = arg_str1(NULL, NULL, "<fast/norm>", "Tester mode, fast: fast check all test cases, norm: run complete test cases.");
    teser_config_args.noise = arg_str1(NULL, NULL, " <all/none/pink/pub>", "noise type, All: test all noise data, none: do not add any noise");
    teser_config_args.snr = arg_str1(NULL, NULL, " <all/none/0/5/10>", "snr, all: test all snr data, none: do not add any noise, 0/5/10: snr number");
    teser_config_args.end = arg_end(1);
    const esp_console_cmd_t tester_cmd = {
        .command = "config",
        .help = "Perf Tester Config",
        .hint = NULL,
        .func = &run_tester_config,
        .argtable = &teser_config_args
    };
    ESP_ERROR_CHECK(esp_console_cmd_register(&tester_cmd));
}

void register_perf_tester_start_cmd(esp_console_cmd_func_t start_func)
{
    const esp_console_cmd_t start_cmd = {
        .command = "start",
        .help = "Start to test",
        .hint = NULL,
        .func = start_func,
    };
    ESP_ERROR_CHECK(esp_console_cmd_register(&start_cmd));
}

perf_tester_config_t* get_perf_tester_config(void)
{
    if (config == NULL) {
        config = (perf_tester_config_t*) malloc(sizeof(perf_tester_config_t));
        strcpy(config->noise, "all");
        strcpy(config->snr, "all");
        strcpy(config->mode, "fast");
    }
    return config;
}

bool check_noise(const char *filename, const char *noise)
{
    // noise
    if (strcmp(noise, "All") == 0 || strcmp(noise, "all") == 0) {
        return true;
    } else if (strcmp(noise, "Pink") == 0 || strcmp(noise, "pink") == 0) {
        if (strstr(filename, "pink") != NULL || strstr(filename, "Pink") != NULL) {
            return true;
        } else {
            return false;
        }
    } else if (strcmp(noise, "Pub") == 0 || strcmp(noise, "pub") == 0) {
        if (strstr(filename, "Pub") != NULL || strstr(filename, "pub") != NULL) {
            return true;
        } else {
            return false;
        }
    } if (strcmp(noise, "None") == 0 || strcmp(noise, "none") == 0) {
        if (strstr(filename, "Silence") != NULL || strstr(filename, "silence") != NULL) {
            return true;
        } else {
            return false;
        }
    }
    return false;
}

bool check_snr(const char *filename, const char *snr)
{
    if (strcmp(snr, "All") == 0 || strcmp(snr, "all") == 0) {
        return true;
    } else if (strcmp(snr, "None") == 0 || strcmp(snr, "none") == 0) {
        return true;
    }
    int snr_num = atoi(snr);
    if (snr_num < -30 || snr_num > 30) {
        return false;
    }

    char name_copy[256];
    char num_copy[128];
    strcpy(name_copy, filename);
    char *rest = NULL;
    char *token = strtok_r(name_copy, "_", &rest);
    int db_num[2];
    int index = 0;

    while (token != NULL) {
        char *end = strstr(token, "dB");
        if (end != NULL) {
            int len = end - token;
            memcpy(num_copy, token, len);
            num_copy[len] = '\0';
            db_num[index] = atoi(num_copy);
            index ++;
        }
        token = strtok_r(NULL, "_", &rest);
    }
    if (index == 2) {
        int file_snr = db_num[0] - db_num[1];
        if (file_snr == snr_num) {
            return true;
        }
    }
    return false;
}