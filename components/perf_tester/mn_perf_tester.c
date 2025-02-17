#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <dirent.h>
#include "assert.h"
#include "wav_decoder.h"
#include "esp_skainet_player.h"
#include "mn_perf_tester.h"
#include "esp_board_init.h"
#include "esp_mn_speech_commands.h"
#include "esp_process_sdkconfig.h"


typedef struct {
    int rb_buffer_size;
    int frame_size;
    char **file_list;           // list of wav files
    char **gt_file_list;        // list of groundtruth csv files of each wav file
    char *log_file;
    char *csv_file;
    int file_num;
    int max_file_num;
    int file_id;
    int nch;
    int sample_rate;
    int64_t wave_time;
    int tester_mem_size;        // total memory for tester, include PSRAM and SRAM
    int tester_sram_size;       // internal SRAM size for tester

    // wakenet
    const esp_afe_sr_iface_t *afe_handle;
    esp_afe_sr_data_t *afe_data;
    afe_config_t *afe_config;

    int *file_wn_det_times;      // wake times for each file
    int *file_wn_miss_times;     // missed wake word times for each file
    float *file_wn_delay_seconds;  // trigger delay of detected wake words for each file
    float *file_wn_max_delay_seconds;  // max trigger delay of detected wake words for each file

    // multinet
    esp_mn_iface_t *multinet;
    model_iface_data_t *mn_data;
    char *mn_name;
    int mn_mem_size;
    int mn_sram_size;
    int mn_active;
    int64_t mn_running_time;

    int *file_mn_correct_times;    // correct detected cmd times for each file
    int *file_mn_incorrect_times;  // incorrect detected cmd times for each file
    int *file_mn_miss_times;       // missed cmd times for each file
    int *file_mn_miss_by_wn_times; // missed cmd times caused by wake net not woken up
    int *file_mn_miss_by_early_timeout_times; // missed cmd times caused by early timeout
    int *file_mn_timeout_times;    // time out times for each file
    int *file_mn_early_timeout_times;    // early time out times for each file
    float *file_mn_delay_seconds;        // trigger delay of correctly detected cmd for each file
    float *file_mn_max_delay_seconds;    // max trigger delay of correctly detected cmd for each file

    // ground truth labels
    int gt_num_regions;
    int *gt_region_type;           // region type, -1: wake word, 0: null, > 0: command id
    float *gt_region_end;          // active speech end timestamp (second) for each region
    float *gt_region_boundary;     // region boundary (equals to the start time of next wake word / command)
    int *file_gt_num_wake;         // number of wake words of each file
    int *file_gt_num_cmd;          // number of commands of each file

    int64_t processed_sample_num;    // number of processed samples
    tester_audio_t audio_type;

    int force_reset;     // manually reset multinet before each command starts
    int test_done;

    // inside section params
    int woke_up;   // whether wakenet is activated
    int current_region_wn_detected;  // whether a wn detection is made in this region
    int current_region_mn_detected;  // whether a mn detection is made in this region
    int early_timeout;  // whether there is a early timeout in current section
    int mn_reseted;  // whether mn is reset in this region
    size_t * file_speech_count;
    size_t * file_vad_speech_count;
    size_t * file_noise_count;
    size_t * file_vad_noise_count;
    size_t * file_vad_speech_change;
    size_t * file_vad_noise_change;
    size_t * file_vad_speech_trigger;
    size_t * file_vad_noise_trigger;


    perf_tester_config_t *config;

} skainet_perf_tester;


int sdcard_scan(void *handle, const char *path, int audio_type)
{
    skainet_perf_tester *tester = handle;
    struct dirent *ret;
    DIR *dir;
    dir = opendir(path);
    int path_len = strlen(path);
    printf("Search files in %s\n", path);
    if (dir != NULL) {

        while ((ret = readdir(dir)) != NULL && tester->file_num < tester->max_file_num) {
            // NULL if reach the end of directory

            if (ret->d_type != 1) { // continue if d_type is not file
                continue;
            }

            int len = strlen(ret->d_name);
            if (len > FATFS_PATH_LENGTH_MAX - path_len - 1) { // continue if name is too long
                continue;
            }

            char *suffix = ret->d_name + len - 4;

            if (audio_type == TESTER_PCM_1CH || audio_type == TESTER_PCM_3CH) {
                if (strcmp(suffix, ".pcm") == 0 || strcmp(suffix, ".PCM") == 0) {
                    memset(tester->file_list[tester->file_num], 0, FATFS_PATH_LENGTH_MAX);
                    memcpy(tester->file_list[tester->file_num], path, path_len);
                    memcpy(tester->file_list[tester->file_num] + path_len, ret->d_name, len + 1);
                    printf("%d -> %s\n", tester->file_num, tester->file_list[tester->file_num]);
                    tester->file_num++;
                }
            } else if (audio_type == TESTER_WAV_1CH || audio_type == TESTER_WAV_3CH) {
                if (strcmp(suffix, ".wav") == 0 || strcmp(suffix, ".WAV") == 0) {
                    memset(tester->file_list[tester->file_num], 0, FATFS_PATH_LENGTH_MAX);
                    memcpy(tester->file_list[tester->file_num], path, path_len);
                    memcpy(tester->file_list[tester->file_num] + path_len, ret->d_name, len + 1);
                    printf("%d -> %s\n", tester->file_num, tester->file_list[tester->file_num]);
                    tester->file_num++;
                }
            }

        }
        closedir(dir);
        printf("Number of files: %d\n", tester->file_num);
    } else {
        printf("Fail to open %s\r\n", path);
    }
    return tester->file_num;
}


void print_mn_report(skainet_perf_tester *tester)
{
    assert(tester != NULL);
    tester->tester_mem_size -= heap_caps_get_free_size(MALLOC_CAP_8BIT);
    tester->tester_sram_size -= heap_caps_get_free_size(MALLOC_CAP_INTERNAL);

    float wave_time = tester->wave_time / tester->sample_rate;
    printf("Tester PSRAM: %d KB\n", (tester->tester_mem_size - tester->tester_sram_size) / 1024);
    printf("Tester SRAM: %d KB\n", tester->tester_sram_size / 1024);

    if (tester->multinet != NULL) {
        float mn_running_time = tester->mn_running_time * 1.0 / 240 / 1000 / 1000;
        printf("MN CPU: %d%%\n", (int)(100 * mn_running_time / wave_time));
        printf("MN PSRAM: %d KB\n", (tester->mn_mem_size - tester->mn_sram_size) / 1024);
        printf("MN SRAM: %d KB\n", tester->mn_sram_size / 1024);
    } else {
        printf("Disable MN Pipeline\n\n");
    }

    if (tester->file_num > 0) {
        int wn_det = 0;
        float wn_delay = 0.0;
        int wn_gt = 0;

        int mn_correct = 0;
        int mn_miss = 0;
        int mn_miss_by_wn = 0;
        int mn_miss_by_timeout = 0;
        int mn_incorrect = 0;
        int mn_timeout = 0;
        int mn_early_timeout = 0;
        float mn_delay = 0.0;
        int mn_gt = 0;

        for (int i = 0; i < tester->file_num; i++) {
            printf("File%d: %s\n", i, tester->file_list[i]);
            printf("File%d, trigger times: %d\n", i, tester->file_wn_det_times[i]);
            printf("File%d, truth times: %d\n", i, tester->file_gt_num_wake[i]);
            printf("File%d, wn averaged delay: %f\n", i, tester->file_wn_delay_seconds[i] / (tester->file_wn_det_times[i] + 0.01));
            printf("File%d, wn max delay: %f\n", i, tester->file_wn_max_delay_seconds[i]);
            wn_det += tester->file_wn_det_times[i];
            wn_gt += tester->file_gt_num_wake[i];
            wn_delay += tester->file_wn_delay_seconds[i];

            printf("File%d, correct commands: %d\n", i, tester->file_mn_correct_times[i]);
            printf("File%d, incorrect commands: %d\n", i, tester->file_mn_incorrect_times[i]);
            printf("File%d, missed commands: %d\n", i, tester->file_mn_miss_times[i]);
            printf("file%d, missed commands caused by wn: %d\n", i, tester->file_mn_miss_by_wn_times[i]);
            printf("file%d, missed commands caused by early time out: %d\n", i, tester->file_mn_miss_by_early_timeout_times[i]);
            printf("File%d, timeout times: %d\n", i, tester->file_mn_timeout_times[i]);
            printf("File%d, early timeout times: %d\n", i, tester->file_mn_early_timeout_times[i]);
            printf("File%d, truth commands: %d\n", i, tester->file_gt_num_cmd[i]);
            printf("File%d, mn averaged delay: %f\n", i, tester->file_mn_delay_seconds[i] / (tester->file_mn_correct_times[i] + 0.01));
            printf("File%d, mn max delay: %f\n", i, tester->file_mn_max_delay_seconds[i]);
            mn_correct += tester->file_mn_correct_times[i];
            mn_miss += tester->file_mn_miss_times[i];
            mn_miss_by_wn += tester->file_mn_miss_by_wn_times[i];
            mn_miss_by_timeout += tester->file_mn_miss_by_early_timeout_times[i];
            mn_incorrect += tester->file_mn_incorrect_times[i];
            mn_timeout += tester->file_mn_timeout_times[i];
            mn_early_timeout += tester->file_mn_early_timeout_times[i];
            mn_gt += tester->file_gt_num_cmd[i];
            mn_delay += tester->file_mn_delay_seconds[i];
        }
        printf("Total trigger times: %d\n", wn_det);
        printf("Total truth times: %d\n", wn_gt);
        printf("Total wn averaged delay: %f\n", wn_delay / wn_det);

        printf("Total correct commands: %d\n", mn_correct);
        printf("Total incorrect commands: %d\n", mn_incorrect);
        printf("Total missed commands: %d\n", mn_miss);
        printf("Total missed commands caused by wn: %d\n", mn_miss_by_wn);
        printf("Total missed commands caused by early timeout: %d\n", mn_miss_by_timeout);
        printf("Total time out: %d\n", mn_timeout);
        printf("Total early time out: %d\n", mn_early_timeout);
        printf("Total truth commands: %d\n", mn_gt);
        printf("Total mn averaged delay: %f\n", mn_delay / mn_correct);
    }

    printf("TEST DONE\n");
}


int read_csv_file(skainet_perf_tester *tester)
{
    FILE *fp = fopen(tester->csv_file, "r");
    printf("csv:%s\n", tester->csv_file);
    tester->file_num = 0;
    if (fp == NULL) {
        return tester->file_num;
    }
    char csv_line[512];
    char *token = NULL;
    char *rest = NULL;
    fgets(csv_line, 512, fp);  //skip csv header
    while (fgets(csv_line, 512, fp) != NULL && tester->file_num < tester->max_file_num) {
        token = strtok_r(csv_line, ",", &rest);
        memset(tester->file_list[tester->file_num], 0, FATFS_PATH_LENGTH_MAX);
        memcpy(tester->file_list[tester->file_num], token, strlen(token));

        printf("%s\n", token);

        if (!check_noise(tester->file_list[tester->file_num], tester->config->noise) ||
            !check_snr(tester->file_list[tester->file_num], tester->config->snr)) {
            continue;
        }
        printf("input:%s\n", tester->file_list[tester->file_num]);

        token = strtok_r(NULL, ",", &rest);
        memset(tester->gt_file_list[tester->file_num], 0, FATFS_PATH_LENGTH_MAX);
        memcpy(tester->gt_file_list[tester->file_num], token, strlen(token));

        token = strtok_r(NULL, ",", &rest);
        tester->file_gt_num_wake[tester->file_num] = atoi(token);

        token = strtok_r(NULL, ",", &rest);
        // tester->file_required_num_wake[tester->file_num] = atoi(token);

        token = strtok_r(NULL, ",", &rest);
        tester->file_gt_num_cmd[tester->file_num] = atoi(token);

        token = strtok_r(NULL, ",", &rest);
        // tester->file_required_num_cmd[tester->file_num] = atoi(token);

        printf("file %d, gt file %s, number of wake words %d, number of commands %d\n",
                tester->file_num,
                tester->gt_file_list[tester->file_num],
                tester->file_gt_num_wake[tester->file_num],
                tester->file_gt_num_cmd[tester->file_num]);

        tester->file_num ++;
    }
    printf("Number of files: %d\n", tester->file_num);
    return tester->file_num;
}


int read_gt_csv_file(skainet_perf_tester *tester, int gt_idx)
{
    FILE *fp = fopen(tester->gt_file_list[gt_idx], "r");
    if (fp == NULL) {
        printf("Failed to open gt file %s\n", tester->gt_file_list[gt_idx]);
        return 0;
    }
    char csv_line[512];
    char *token = NULL;
    fgets(csv_line, 512, fp);  //skip csv header

    int num_regions = tester->file_gt_num_cmd[gt_idx] + tester->file_gt_num_wake[gt_idx];
    tester->gt_num_regions = num_regions;

    if (tester->gt_region_type != NULL) {
        free(tester->gt_region_type);
    }
    tester->gt_region_type = calloc(num_regions, sizeof(int));

    if (tester->gt_region_end != NULL) {
        free(tester->gt_region_end);
    }
    tester->gt_region_end = calloc(num_regions, sizeof(float));

    if (tester->gt_region_boundary != NULL) {
        free(tester->gt_region_boundary);
    }
    tester->gt_region_boundary = calloc(num_regions, sizeof(float));

    int idx = 0;
    while (fgets(csv_line, 512, fp) != NULL) {
        token = strtok(csv_line, ",");
        tester->gt_region_type[idx] = atoi(token);
        token = strtok(NULL, ",");
        tester->gt_region_end[idx] = atof(token);
        token = strtok(NULL, ",");
        tester->gt_region_boundary[idx] = atof(token);
        idx++;
    }
    return 1;
}


void wav_feed_task(void *arg)
{
    printf("Create wav feed task ...\n");
    skainet_perf_tester *tester = arg;
    const esp_afe_sr_iface_t *afe_handle = tester->afe_handle;
    esp_afe_sr_data_t *afe_data = tester->afe_data;
    void *wav_decoder = NULL;
    int sample_rate = tester->sample_rate;
    int frame_size = tester->frame_size;
    int nch = tester->nch;
    int file_nch = 0;

    int i2s_buffer_size = frame_size * (nch + 1) * sizeof(int16_t);

    int16_t *i2s_buffer = calloc(frame_size * (nch + 1), sizeof(int16_t)); // nch channel MIC data and one channel reference data
    tester->wave_time = 0;

    for (int i = 0; i < tester->file_num; i++) {
        wav_decoder = wav_decoder_open(tester->file_list[i]);
        file_nch = wav_decoder_get_channel(wav_decoder);

        if (wav_decoder == NULL) {
            printf("can not find %s, play next song\n", tester->file_list[i]);
            continue;
        } else if (wav_decoder_get_sample_rate(wav_decoder) != sample_rate) {
            printf("The sample rate of %s does not meet the requirements, please resample to %d\n",
                   tester->file_list[i], sample_rate);
            wav_decoder_close(wav_decoder);
            continue;
        } else if (file_nch != nch + 1) {

            printf("The channel of %s does not meet the requirements(n=%d), please input %d channel MIC data and one channel reference data\n",
                   tester->file_list[i], file_nch, nch);
            wav_decoder_close(wav_decoder);
            continue;
        } else {
            printf("start to process %s\n", tester->file_list[i]);
        }

        tester->file_id = i;
        tester->file_wn_det_times[i] = 0;
        tester->file_wn_miss_times[i] = 0;
        tester->file_wn_delay_seconds[i] = 0.0;
        tester->file_wn_max_delay_seconds[i] = 0.0;
        tester->file_mn_correct_times[i] = 0;
        tester->file_mn_incorrect_times[i] = 0;
        tester->file_mn_timeout_times[i] = 0;
        tester->file_mn_early_timeout_times[i] = 0;
        tester->file_mn_miss_times[i] = 0;
        tester->file_mn_miss_by_wn_times[i] = 0;
        tester->file_mn_miss_by_early_timeout_times[i] = 0;
        tester->file_mn_delay_seconds[i] = 0.0;
        tester->file_mn_max_delay_seconds[i] = 0.0;

        int out_samples = 0;
        int size = i2s_buffer_size;

        while (1) {
            size = wav_decoder_run(wav_decoder, (unsigned char *)i2s_buffer, i2s_buffer_size);
            out_samples += frame_size;

            if (size == i2s_buffer_size) {
                afe_handle->feed(afe_data, i2s_buffer);
                vTaskDelay(32 / portTICK_PERIOD_MS);
            } else {
                // wav decoder free
                vTaskDelay(1000 / portTICK_PERIOD_MS);
                wav_decoder_close(wav_decoder);
                wav_decoder = NULL;
                vTaskDelay(1000 / portTICK_PERIOD_MS);
                break;
            }
        }

        tester->wave_time += out_samples;
    }

    tester->test_done = 1;
    print_mn_report(tester);
    vTaskDelete(NULL);
}


void detect_task(void *arg)
{
    printf("Create detect task ...\n");
    skainet_perf_tester *tester = arg;
    int afe_chunksize = tester->afe_handle->get_fetch_chunksize(tester->afe_data);

    int mn_chunksize = tester->multinet->get_samp_chunksize(tester->mn_data);

    assert(mn_chunksize == afe_chunksize);

    int file_id = 0;
    read_gt_csv_file(tester, file_id);
    tester->processed_sample_num = 0;
    tester->mn_running_time = 0;
    uint32_t c0, c1;

    int gt_idx = 0;
    float curr_time_s;
    tester->woke_up = 0;
    tester->current_region_mn_detected = 0;
    tester->current_region_wn_detected = 0;
    tester->early_timeout = 0;
    tester->mn_reseted = 0;

    while (1) {
        afe_fetch_result_t* res = tester->afe_handle->fetch(tester->afe_data);
        tester->processed_sample_num += mn_chunksize;
        if (!res || res->ret_value == ESP_FAIL) {
            break;
        }

        curr_time_s = (float) tester->processed_sample_num / 16000.0;

        if (curr_time_s > tester->gt_region_boundary[gt_idx]) {
            // we need to move to next region
            // printf("move from region %d to %d, %d\n", gt_idx, gt_idx+1, current_region_detected);
            if (tester->gt_region_type[gt_idx] == -1 && tester->current_region_wn_detected == 0) {
                // current region is wake word but not detected
                tester->file_wn_miss_times[file_id]++;
                tester->woke_up = 0;
            } else if (tester->gt_region_type[gt_idx] > 0 && tester->current_region_mn_detected == 0) {
                // current region is command but not detected
                if (tester->mn_active == 1) {
                    // mn still active
                    // printf("command miss, mn active\n");
                    tester->file_mn_miss_times[file_id]++;
                } else {
                    // mn not active, three cases
                    if (tester->early_timeout == 1) {
                        // case 1 early timeout
                        // printf("command miss by early timeout\n");
                        tester->file_mn_miss_by_early_timeout_times[file_id]++;
                    } else if (tester->woke_up == 0) {
                        // case 2 wake net is not triggered
                        // printf("command miss by wn\n");
                        tester->file_mn_miss_by_wn_times[file_id]++;
                    } else {
                        // case 3, normal time out, this is the last command in region
                        // printf("command miss, mn inactive\n");
                        tester->file_mn_miss_times[file_id]++;
                    }
                }
            }
            gt_idx += 1;
            curr_time_s = (float) tester->processed_sample_num / 16000.0;

            // reset some parameters
            tester->mn_reseted = 0;
            tester->current_region_mn_detected = 0;
            tester->current_region_wn_detected = 0;

            if (tester->gt_region_type[gt_idx] == -1) {
                // new region is wn region, it is a new section
                tester->woke_up = 0;
                tester->early_timeout = 0;
            }
        } else if (tester->force_reset == 1 && tester->mn_reseted == 0 && tester->gt_region_type[gt_idx] > 0 && tester->gt_region_boundary[gt_idx] - curr_time_s <= 1.0) {
            // if force reset, current region has not been reseted yet and current time is within 1 sec to the boundary
            tester->multinet->clean(tester->mn_data);
            tester->mn_reseted = 1;
            if (gt_idx < tester->gt_num_regions - 1 && tester->gt_region_type[gt_idx+1] == -1 && tester->mn_active == 1){
                // if next region is wake word and mn hasn't time out, force time out
                // printf("force timeout\n");
                tester->mn_active = 0;
                tester->woke_up = 1;  // this section is woke up because mn_active was 1
                tester->early_timeout = 0;  // this is the last region in section, so not a early timeout
            }
        }

        // the curr_time_s should never exceed the last region boundary
        // make sure the last region boundary in csv is at least number of total length of current audio
        assert(curr_time_s <= tester->gt_region_boundary[gt_idx]);

        if (res->wakeup_state == WAKENET_DETECTED) {
            tester->woke_up = 1;
            if (tester->gt_region_type[gt_idx] == -1) {
                // wake word region
                if (curr_time_s < (tester->gt_region_end[gt_idx] - 1.0)) {
                    // detected too early, one second earlier than when speech actual ends
                    printf("wn detected too early, %f, %f\n",curr_time_s, tester->gt_region_end[gt_idx]);
                } else {
                    tester->file_wn_det_times[file_id]++;
                    float delay = curr_time_s - tester->gt_region_end[gt_idx];
                    tester->file_wn_delay_seconds[file_id] += delay;
                    if (delay > tester->file_wn_max_delay_seconds[file_id]) {
                        tester->file_wn_max_delay_seconds[file_id] = delay;
                    }
                    tester->current_region_wn_detected = 1;
                    // printf("wn detected, %f, %f\n",curr_time_s, tester->gt_region_end[gt_idx]);
                }
            } else {
                printf("wake word is detected at a command region. %d\n", gt_idx);
            }
            tester->multinet->clean(tester->mn_data);
        } else if (res->wakeup_state == WAKENET_CHANNEL_VERIFIED) {
            tester->mn_active = 1;
        }

        if (tester->mn_active == 1) {
            c0 = esp_cpu_get_cycle_count();
            esp_mn_state_t mn_state = tester->multinet->detect(tester->mn_data, res->data);
            c1 = esp_cpu_get_cycle_count();
            tester->mn_running_time += c1 - c0;
            if (mn_state == ESP_MN_STATE_DETECTED) {
                esp_mn_results_t *mn_result = tester->multinet->get_results(tester->mn_data);
                if (tester->gt_region_type[gt_idx] > 0) {
                    if (curr_time_s < (tester->gt_region_end[gt_idx] - 1.0)) {
                        // detected too early, one second earlier than when speech actual ends
                        printf("mn detected too early, classified as incorrect\n");
                        tester->file_mn_incorrect_times[file_id]++;
                    } else {
                        // cmd region
                        if (mn_result->command_id[0] == tester->gt_region_type[gt_idx]) {
                            // correct command
                            tester->file_mn_correct_times[file_id]++;
                            float delay = curr_time_s - tester->gt_region_end[gt_idx];
                            tester->file_mn_delay_seconds[file_id] += delay;
                            if (delay > tester->file_mn_max_delay_seconds[file_id]) {
                                printf("new max delay %f, idx %d (%f -> %f)\n", delay, gt_idx, curr_time_s, tester->gt_region_end[gt_idx]);
                                tester->file_mn_max_delay_seconds[file_id] = delay;
                            }
                            // printf("command correct, %f %f %f\n", curr_time_s, tester->gt_region_end[gt_idx], tester->file_mn_delay_seconds[file_id]);
                        } else {
                            // incorrect command
                            tester->file_mn_incorrect_times[file_id]++;
                            // printf("command incorrect, %f %f %f\n", curr_time_s, tester->gt_region_end[gt_idx], tester->file_mn_delay_seconds[file_id]);
                        }
                        tester->current_region_mn_detected = 1;
                    }
                } else {
                    printf("command is detected at a wake word region\n");
                }
                tester->multinet->clean(tester->mn_data);
            }

            if (mn_state == ESP_MN_STATE_TIMEOUT) {
                tester->mn_active = 0;
                tester->multinet->clean(tester->mn_data);
                tester->file_mn_timeout_times[file_id]++;
                if (gt_idx < tester->gt_num_regions - 1 && tester->gt_region_type[gt_idx+1] > 0 && tester->gt_region_type[gt_idx] > 0) {
                    // next region is not wake word, means this time out is early
                    // printf("early time out, %f\n", curr_time_s);
                    tester->file_mn_early_timeout_times[file_id]++;
                    tester->early_timeout = 1;
                } else {
                    // printf("time out, %f\n", curr_time_s);
                    tester->early_timeout = 0;
                }
            }
        }

        if (file_id != tester->file_id || tester->test_done) {
            // finish up last region of current file first
            if (tester->gt_region_type[gt_idx] == -1 && tester->current_region_wn_detected == 0) {
                // current region is wake word but not detected
                tester->file_wn_miss_times[file_id]++;
            } else if (tester->gt_region_type[gt_idx] > 0 && tester->current_region_mn_detected == 0) {
                // current region is command but not detected
                if (tester->mn_active == 1) {
                    // mn still active
                    // printf("command miss, mn active\n");
                    tester->file_mn_miss_times[file_id]++;
                } else {
                    // mn not active, three cases
                    if (tester->early_timeout == 1) {
                        // case 1 early timeout
                        // printf("command miss by early timeout\n");
                        tester->file_mn_miss_by_early_timeout_times[file_id]++;
                    } else if (tester->woke_up == 0) {
                        // case 2 wake net is not triggered
                        // printf("command miss by wn\n");
                        tester->file_mn_miss_by_wn_times[file_id]++;
                    } else {
                        // case 3, normal time out, this is the last command in region
                        // printf("command miss, mn inactive\n");
                        tester->file_mn_miss_times[file_id]++;
                    }
                }
            }
        }

        if (file_id != tester->file_id) {
            // new file
            file_id = tester->file_id;
            // reset ground truth
            gt_idx = 0;
            tester->processed_sample_num = 0;
            read_gt_csv_file(tester, file_id);
            tester->mn_active = 0;
            tester->multinet->clean(tester->mn_data);
            tester->current_region_mn_detected = 0;
            tester->current_region_wn_detected = 0;
            tester->woke_up = 0;
            tester->early_timeout = 0;
        } else if (tester->test_done) {
            // finish up last region of current file first
            break;
        }
    }
    if (tester->mn_data) {
        tester->multinet->destroy(tester->mn_data);
        tester->mn_data = NULL;
    }
    vTaskDelete(NULL);
}


void add_test_commands(void *arg)
{
    printf("Clear pre-defined command list and create one for perf_tester.\n");
    esp_mn_commands_clear();
    skainet_perf_tester *tester = arg;
    if (strcmp(tester->multinet->get_language(tester->mn_data), ESP_MN_ENGLISH) == 0) {
        esp_mn_commands_add(1, "TELL ME A JOKE");
        esp_mn_commands_add(2, "SING A SONG");
        esp_mn_commands_add(3, "PLAY NEWS CHANNEL");
        esp_mn_commands_add(4, "TURN ON MY SOUNDBOX");
        esp_mn_commands_add(5, "TURN OFF MY SOUNDBOX");
        esp_mn_commands_add(5, "TURN OF MY SOUNDBOX");
        esp_mn_commands_add(6, "HIGHEST VOLUME");
        esp_mn_commands_add(7, "LOWEST VOLUME");
        esp_mn_commands_add(8, "INCREASE THE VOLUME");
        esp_mn_commands_add(9, "DECREASE THE VOLUME");
        esp_mn_commands_add(10, "TURN ON THE TV");
        esp_mn_commands_add(11, "TURN OFF THE TV");
        esp_mn_commands_add(11, "TURN OF THE TV");
        esp_mn_commands_add(12, "MAKE ME A TEA");
        esp_mn_commands_add(13, "MAKE ME A COFFEE");
        esp_mn_commands_add(14, "TURN ON THE LIGHT");
        esp_mn_commands_add(15, "TURN OFF THE LIGHT");
        esp_mn_commands_add(15, "TURN OF THE LIGHT");
        esp_mn_commands_add(16, "CHANGE THE COLOR TO RED");
        esp_mn_commands_add(17, "CHANGE THE COLOR TO GREEN");
        esp_mn_commands_add(18, "TURN ON ALL THE LIGHTS");
        esp_mn_commands_add(19, "TURN OFF ALL THE LIGHTS");
        esp_mn_commands_add(19, "TURN OF ALL THE LIGHTS");
        esp_mn_commands_add(20, "TURN ON THE AIR CONDITIONER");
        esp_mn_commands_add(21, "TURN OFF THE AIR CONDITIONER");
        esp_mn_commands_add(21, "TURN OF THE AIR CONDITIONER");
        esp_mn_commands_add(22, "SET THE TEMPERATURE TO SIXTEEN DEGREES");
        esp_mn_commands_add(23, "SET THE TEMPERATURE TO SEVENTEEN DEGREES");
        esp_mn_commands_add(24, "SET THE TEMPERATURE TO EIGHTEEN DEGREES");
        esp_mn_commands_add(25, "SET THE TEMPERATURE TO NINETEEN DEGREES");
        esp_mn_commands_add(26, "SET THE TEMPERATURE TO TWENTY DEGREES");
        esp_mn_commands_add(27, "SET THE TEMPERATURE TO TWENTY ONE DEGREES");
        esp_mn_commands_add(28, "SET THE TEMPERATURE TO TWENTY TWO DEGREES");
        esp_mn_commands_add(29, "SET THE TEMPERATURE TO TWENTY THREE DEGREES");
        esp_mn_commands_add(30, "SET THE TEMPERATURE TO TWENTY FOUR DEGREES");
        esp_mn_commands_add(31, "SET THE TEMPERATURE TO TWENTY FIVE DEGREES");
        esp_mn_commands_add(32, "SET THE TEMPERATURE TO TWENTY SIX DEGREES");
        esp_mn_commands_add(33, "LOWEST FAN SPEED");
        esp_mn_commands_add(34, "MEDIUM FAN SPEED");
        esp_mn_commands_add(35, "HIGHEST FAN SPEED");
        esp_mn_commands_add(36, "AUTO ADJUST THE FAN SPEED");
        esp_mn_commands_add(37, "DECREASE THE FAN SPEED");
        esp_mn_commands_add(38, "INCREASE THE FAN SPEED");
        esp_mn_commands_add(39, "INCREASE THE TEMPERATURE");
        esp_mn_commands_add(40, "DECREASE THE TEMPERATURE");
        esp_mn_commands_add(41, "COOLING MODE");
        esp_mn_commands_add(42, "HEATING MODE");
        esp_mn_commands_add(43, "VENTILATION MODE");
        esp_mn_commands_add(44, "DEHUMIDIFY MODE");
    } else {
        esp_mn_commands_add(25, "da kai kong tiao");
        esp_mn_commands_add(74, "tiao dao er shi ba du");
        esp_mn_commands_add(75, "tiao dao er shi du");
        esp_mn_commands_add(79, "tiao dao er shi qi du");
        esp_mn_commands_add(85, "tiao dao shi ba du");
        esp_mn_commands_add(88, "tiao dao shi qi du");
        esp_mn_commands_add(93, "tiao di yi du");
        esp_mn_commands_add(98, "tiao gao yi du");
        esp_mn_commands_add(120, "feng xiang shang chui");
        esp_mn_commands_add(121, "feng xiang xia chui");
        esp_mn_commands_add(142, "guan bi kong tiao");
        esp_mn_commands_add(288, "zhi leng mo shi");
        esp_mn_commands_add(289, "zhi re mo shi");
        esp_mn_commands_add(301, "zui da feng su");
        esp_mn_commands_add(304, "zui xiao feng su");
    }
    esp_mn_commands_update();
    esp_mn_commands_print();
}


void offline_mn_tester(const char *csv_file,
                       const char *log_file,
                       const esp_afe_sr_iface_t *afe_handle,
                       afe_config_t *afe_config,
                       esp_mn_iface_t *multinet,
                       char *mn_coeff,
                       int audio_type,
                       perf_tester_config_t *config)
{
    skainet_perf_tester *tester = malloc(sizeof(skainet_perf_tester));
    tester->config = config;
    // ringbuffer init
    tester->tester_mem_size = heap_caps_get_free_size(MALLOC_CAP_8BIT);
    tester->tester_sram_size = heap_caps_get_free_size(MALLOC_CAP_INTERNAL);

    int m1 = 0, m2 = 0;
    int sm1 = 0, sm2 = 0;
    tester->rb_buffer_size = 4096 * 2;

    // file list init
    tester->max_file_num = 50;
    tester->file_id = 0;
    tester->file_num = 0;
    tester->file_list = malloc(sizeof(char *) * tester->max_file_num);
    tester->gt_file_list = malloc(sizeof(char *) * tester->max_file_num);
    tester->file_gt_num_wake = malloc(sizeof(int) * tester->max_file_num);
    tester->file_gt_num_cmd = malloc(sizeof(int) * tester->max_file_num);

    tester->file_wn_det_times = calloc(tester->max_file_num, sizeof(int));
    tester->file_wn_miss_times = calloc(tester->max_file_num, sizeof(int));
    tester->file_wn_delay_seconds = calloc(tester->max_file_num, sizeof(float));
    tester->file_wn_max_delay_seconds = calloc(tester->max_file_num, sizeof(float));

    tester->file_mn_correct_times = calloc(tester->max_file_num, sizeof(int));
    tester->file_mn_incorrect_times = calloc(tester->max_file_num, sizeof(int));
    tester->file_mn_miss_times = calloc(tester->max_file_num, sizeof(int));
    tester->file_mn_miss_by_wn_times = calloc(tester->max_file_num, sizeof(int));
    tester->file_mn_miss_by_early_timeout_times = calloc(tester->max_file_num, sizeof(int));
    tester->file_mn_timeout_times = calloc(tester->max_file_num, sizeof(int));
    tester->file_mn_early_timeout_times = calloc(tester->max_file_num, sizeof(int));
    tester->file_mn_delay_seconds = calloc(tester->max_file_num, sizeof(float));
    tester->file_mn_max_delay_seconds = calloc(tester->max_file_num, sizeof(float));

    tester->audio_type = audio_type;
    for (int i = 0; i < tester->max_file_num; i++) {
        tester->file_list[i] = calloc(FATFS_PATH_LENGTH_MAX, sizeof(char));
        tester->gt_file_list[i] = calloc(FATFS_PATH_LENGTH_MAX, sizeof(char));
    }

    tester->gt_region_type = NULL;
    tester->gt_region_end = NULL;
    tester->gt_region_boundary = NULL;

    tester->csv_file = (char *)csv_file;
    read_csv_file(tester);
    tester->log_file = (char *) log_file;

    tester->force_reset = 0;
    tester->test_done = 0;

    // init AFE
    // afe_config->afe_mode = SR_MODE_HIGH_PERF;
    tester->afe_config = afe_config;
    tester->afe_handle = afe_handle;
    tester->afe_data = afe_handle->create_from_config(afe_config);
    tester->frame_size = afe_handle->get_feed_chunksize(tester->afe_data);
    tester->sample_rate = afe_handle->get_samp_rate(tester->afe_data);
    tester->nch = afe_handle->get_channel_num(tester->afe_data);

    // the memory before MN init
    m1 = heap_caps_get_free_size(MALLOC_CAP_8BIT);
    sm1 = heap_caps_get_free_size(MALLOC_CAP_INTERNAL);

    // mn init
    tester->mn_name = mn_coeff;
    tester->multinet = multinet;
    tester->mn_data = multinet->create(mn_coeff, 12000);
    esp_mn_commands_update_from_sdkconfig(tester->multinet, tester->mn_data);

    // add commands for testing, use the default phonemes for mn7_en test, as flite_g2p leads to worse performance
    if (strcmp(tester->mn_name, "mn7_en") != 0) {
        add_test_commands(tester);
    }

    // the memory after AFE init
    m2 = heap_caps_get_free_size(MALLOC_CAP_8BIT);
    sm2 = heap_caps_get_free_size(MALLOC_CAP_INTERNAL);
    tester->mn_mem_size = m1 - m2;               // the memory size of mn init
    tester->mn_sram_size = sm1 - sm2;
    tester->mn_active = 0;

    // running time init
    tester->wave_time = 0;
    tester->mn_running_time = 0;

    if (tester->file_num == 0) {
        print_mn_report(tester);
        return ;
    }

    // printf("The memory info after init:\n");
    if (audio_type == TESTER_WAV_3CH) {
        xTaskCreatePinnedToCore(&wav_feed_task, "wav_feed_task", 4 * 1024, (void *)tester, 8, NULL, 1);
    }

    if (audio_type == TESTER_WAV_3CH) {
        xTaskCreatePinnedToCore(&detect_task, "detect_task", 4 * 1024, (void *)tester, 8, NULL, 0);
        vTaskDelay(2000 / portTICK_PERIOD_MS);
    }
}


void print_vad_report(skainet_perf_tester *tester)
{
    assert(tester != NULL);
    tester->tester_mem_size -= heap_caps_get_free_size(MALLOC_CAP_8BIT);
    tester->tester_sram_size -= heap_caps_get_free_size(MALLOC_CAP_INTERNAL);

    // float wave_time = tester->wave_time / tester->sample_rate;
    printf("Tester PSRAM: %d KB\n", (tester->tester_mem_size - tester->tester_sram_size) / 1024);
    printf("Tester SRAM: %d KB\n", tester->tester_sram_size / 1024);

    if (tester->file_num > 0) {
        for (int i = 0; i < tester->file_num; i++) {
            printf("File%d: %s\n", i, tester->file_list[i]);
            printf("File%d, speech count: %d\n", i, tester->file_speech_count[i]);
            printf("File%d, noise count: %d\n", i, tester->file_noise_count[i]);
            printf("File%d, speech state change: %d\n", i, tester->file_vad_speech_change[i]);
            printf("File%d, speech trigger: %d\n", i, tester->file_vad_speech_trigger[i]);
            printf("File%d, noise state change: %d\n", i, tester->file_vad_noise_change[i]);
            printf("File%d, noise trigger: %d\n", i, tester->file_vad_noise_trigger[i]);
            printf("File%d, vad speech count: %d, accuracy:%f\n", i, tester->file_vad_speech_count[i], tester->file_vad_speech_count[i]*1.0/tester->file_speech_count[i]);
            printf("File%d, vad noise count: %d, accuracy:%f\n", i, tester->file_vad_noise_count[i], tester->file_vad_noise_count[i]*1.0/tester->file_noise_count[i]);
        
        }
    }

    printf("TEST DONE\n");
}
void vad_feed_task(void *arg)
{
    printf("Create vad feed task ...\n");
    skainet_perf_tester *tester = arg;
    const esp_afe_sr_iface_t *afe_handle = tester->afe_handle;
    esp_afe_sr_data_t *afe_data = tester->afe_data;
    void *wav_decoder = NULL;
    int sample_rate = tester->sample_rate;
    int frame_size = tester->frame_size;
    int nch = tester->nch;
    int file_nch = 0;

    int i2s_buffer_size = frame_size * (nch + 1) * sizeof(int16_t);

    int16_t *i2s_buffer = calloc(frame_size * (nch + 1), sizeof(int16_t)); // nch channel MIC data and one channel reference data
    tester->wave_time = 0;

    for (int i = 0; i < tester->file_num; i++) {
        wav_decoder = wav_decoder_open(tester->file_list[i]);
        file_nch = wav_decoder_get_channel(wav_decoder);

        if (wav_decoder == NULL) {
            printf("can not find %s, play next song\n", tester->file_list[i]);
            continue;
        } else if (wav_decoder_get_sample_rate(wav_decoder) != sample_rate) {
            printf("The sample rate of %s does not meet the requirements, please resample to %d\n",
                   tester->file_list[i], sample_rate);
            wav_decoder_close(wav_decoder);
            continue;
        } else if (file_nch != nch) {

            printf("The channel of %s does not meet the requirements(n=%d), please input %d channel MIC data and one channel reference data\n",
                   tester->file_list[i], file_nch, nch);
            wav_decoder_close(wav_decoder);
            continue;
        } else {
            printf("start to process %s\n", tester->file_list[i]);
        }

        tester->file_id = i;
        tester->file_speech_count[i] = 0;
        tester->file_noise_count[i] = 0;
        tester->file_vad_noise_count[i] = 0.0;
        tester->file_vad_speech_count[i] = 0.0;
        tester->file_vad_noise_change[i] = 0.0;
        tester->file_vad_speech_change[i] = 0.0;
        tester->file_vad_speech_trigger[i] = 0.0;
        tester->file_vad_noise_trigger[i] = 0.0;

        int out_samples = 0;
        int size = i2s_buffer_size;

        while (1) {
            size = wav_decoder_run(wav_decoder, (unsigned char *)i2s_buffer, i2s_buffer_size);
            out_samples += frame_size;

            if (size == i2s_buffer_size) {
                afe_handle->feed(afe_data, i2s_buffer);
                vTaskDelay(32 / portTICK_PERIOD_MS);
            } else {
                // wav decoder free
                vTaskDelay(1000 / portTICK_PERIOD_MS);
                wav_decoder_close(wav_decoder);
                wav_decoder = NULL;
                vTaskDelay(1000 / portTICK_PERIOD_MS);
                break;
            }
        }

        printf("File%d: %s\n", i, tester->file_list[i]);
        printf("File%d, speech count: %d\n", i, tester->file_speech_count[i]);
        printf("File%d, noise count: %d\n", i, tester->file_noise_count[i]);
        printf("File%d, speech state change: %d\n", i, tester->file_vad_speech_change[i]);
        printf("File%d, speech trigger: %d\n", i, tester->file_vad_speech_trigger[i]);
        printf("File%d, noise state change: %d\n", i, tester->file_vad_noise_change[i]);
        printf("File%d, noise trigger: %d\n", i, tester->file_vad_noise_trigger[i]);
        printf("File%d, vad speech count: %d, accuracy:%f\n", i, tester->file_vad_speech_count[i], tester->file_vad_speech_count[i]*1.0/tester->file_speech_count[i]);
        printf("File%d, vad noise count: %d, accuracy:%f\n", i, tester->file_vad_noise_count[i], tester->file_vad_noise_count[i]*1.0/tester->file_noise_count[i]);
    
        tester->wave_time += out_samples;
    }

    tester->test_done = 1;
    print_vad_report(tester);
    vTaskDelete(NULL);
}

void save_vad_states(int8_t* states, int size, char* file_name) 
{
    char* state_file = (char*)calloc(FATFS_PATH_LENGTH_MAX, sizeof(char));
    sprintf(state_file, "%s_states.bin", file_name);
    FILE *fp = fopen(state_file, "wb");
    if (fp == NULL) {
        printf("Failed to open file %s for writing\n", state_file);
        return;
    } else {
        printf("prepare to write vad states");
    }
    fwrite(states, sizeof(int8_t), size, fp);
    fclose(fp);
}

void vad_detect_task(void *arg)
{
    printf("Create detect task ...\n");
    skainet_perf_tester *tester = arg;
    int afe_chunksize = tester->afe_handle->get_fetch_chunksize(tester->afe_data);

    int file_id = 0;
    read_gt_csv_file(tester, file_id);
    tester->processed_sample_num = 0;
    tester->mn_running_time = 0;

    int gt_idx = 0;
    float curr_time_s;
    tester->woke_up = 0;
    tester->current_region_mn_detected = 0;
    tester->current_region_wn_detected = 0;
    tester->early_timeout = 0;
    tester->mn_reseted = 0;
    int speech_seg_flag = 0;
    int noise_seg_flag = 0;
    vad_state_t last_state = VAD_SILENCE;
    int8_t *file_vad_states = (int8_t *) malloc(60*60*60*sizeof(int8_t));
    int file_chunk = 0;

    while (1) {
        afe_fetch_result_t* res = tester->afe_handle->fetch(tester->afe_data);
        if (res->ret_value == ESP_FAIL) {
            continue;;
        }
        vad_state_t state = res->vad_state;
        if (state == VAD_SPEECH) {
            file_vad_states[file_chunk] = 1;
        } else {
            file_vad_states[file_chunk] = 0;
        }
        file_chunk++;

        tester->processed_sample_num += afe_chunksize;
        if (!res || res->ret_value == ESP_FAIL) {
            break;
        }

        curr_time_s = (float) tester->processed_sample_num / 16000.0;
        if (curr_time_s <= tester->gt_region_end[gt_idx]) {
            // speech
            if (gt_idx > 0) {
                tester->file_speech_count[file_id] += 1;
                if (state == VAD_SPEECH) {
                    tester->file_vad_speech_count[file_id] += 1;
                    printf("speech->speech\n");
                    speech_seg_flag = 1;
                } else {
                    printf("speech->noise\n");
                }
                if (state != last_state && state == VAD_SILENCE) {
                    tester->file_vad_speech_change[file_id] += 1;
                    // printf("------state change1-----, noise->speech\n");
                }
            }
        } else if (curr_time_s < tester->gt_region_boundary[gt_idx]) {
            // noise
            tester->file_noise_count[file_id] += 1;

            if (state == VAD_SILENCE) {
                tester->file_vad_noise_count[file_id] += 1;
                // printf("noise->noise\n");
                noise_seg_flag = 1;
            } else {
                // printf("noise->speech\n");
            }

            if (state != last_state && state == VAD_SPEECH) {
                tester->file_vad_noise_change[file_id] += 1;
                // printf("------state change2-----, speech->noise\n");
            }
        } else {
            gt_idx += 1;
            if (speech_seg_flag) {
                tester->file_vad_speech_trigger[file_id]  += 1;
            }
            if (noise_seg_flag) {
                tester->file_vad_noise_trigger[file_id]  += 1;
            }
            speech_seg_flag = 0;
            noise_seg_flag = 1;
        }
        last_state = state;

        // the curr_time_s should never exceed the last region boundary
        // make sure the last region boundary in csv is at least number of total length of current audio
        assert(curr_time_s <= tester->gt_region_boundary[gt_idx]);
        if (file_id != tester->file_id) {
            save_vad_states(file_vad_states, file_chunk, tester->file_list[file_id]);
            file_chunk = 0;
            // new file
            file_id = tester->file_id;
            // reset ground truth
            gt_idx = 0;
            tester->processed_sample_num = 0;
            read_gt_csv_file(tester, file_id);
        } else if (tester->test_done) {
            // finish up last region of current file first
            break;
        }
    }
    vTaskDelete(NULL);
}


void offline_vad_tester(const char *csv_file,
                       const char *log_file,
                       const esp_afe_sr_iface_t *afe_handle,
                       afe_config_t *afe_config,
                       int audio_type,
                       perf_tester_config_t *config)
{
    skainet_perf_tester *tester = malloc(sizeof(skainet_perf_tester));
    tester->config = config;
    // ringbuffer init
    tester->tester_mem_size = heap_caps_get_free_size(MALLOC_CAP_8BIT);
    tester->tester_sram_size = heap_caps_get_free_size(MALLOC_CAP_INTERNAL);
    tester->rb_buffer_size = 4096 * 2;

    // file list init
    tester->max_file_num = 50;
    tester->file_id = 0;
    tester->file_num = 0;
    tester->file_list = malloc(sizeof(char *) * tester->max_file_num);
    tester->gt_file_list = malloc(sizeof(char *) * tester->max_file_num);
    tester->file_gt_num_wake = malloc(sizeof(int) * tester->max_file_num);
    tester->file_gt_num_cmd = malloc(sizeof(int) * tester->max_file_num);

    tester->file_speech_count = calloc(tester->max_file_num, sizeof(int));
    tester->file_noise_count = calloc(tester->max_file_num, sizeof(int));
    tester->file_vad_speech_count = calloc(tester->max_file_num, sizeof(float));
    tester->file_vad_noise_count = calloc(tester->max_file_num, sizeof(float));
    tester->file_vad_speech_change = calloc(tester->max_file_num, sizeof(float));
    tester->file_vad_noise_change = calloc(tester->max_file_num, sizeof(float));
    tester->file_vad_speech_trigger = calloc(tester->max_file_num, sizeof(float));
    tester->file_vad_noise_trigger = calloc(tester->max_file_num, sizeof(float));

    tester->audio_type = audio_type;
    for (int i = 0; i < tester->max_file_num; i++) {
        tester->file_list[i] = calloc(FATFS_PATH_LENGTH_MAX, sizeof(char));
        tester->gt_file_list[i] = calloc(FATFS_PATH_LENGTH_MAX, sizeof(char));
    }

    tester->gt_region_type = NULL;
    tester->gt_region_end = NULL;
    tester->gt_region_boundary = NULL;

    tester->csv_file = (char *)csv_file;
    read_csv_file(tester);
    tester->log_file = (char *) log_file;

    tester->force_reset = 0;
    tester->test_done = 0;

    // init AFE
    // afe_config->afe_mode = SR_MODE_HIGH_PERF;
    tester->afe_config = afe_config;
    tester->afe_handle = afe_handle;
    tester->afe_data = afe_handle->create_from_config(afe_config);
    tester->frame_size = afe_handle->get_feed_chunksize(tester->afe_data);
    tester->sample_rate = afe_handle->get_samp_rate(tester->afe_data);
    tester->nch = afe_handle->get_channel_num(tester->afe_data);


    // running time init
    tester->wave_time = 0;
    tester->mn_running_time = 0;

    if (tester->file_num == 0) {
        print_mn_report(tester);
        return ;
    }

    // printf("The memory info after init:\n");
    if (audio_type == TESTER_WAV_3CH) {
        xTaskCreatePinnedToCore(&vad_feed_task, "vad_feed_task", 4 * 1024, (void *)tester, 8, NULL, 1);
    }

    if (audio_type == TESTER_WAV_3CH) {
        xTaskCreatePinnedToCore(&vad_detect_task, "vad_detect_task", 4 * 1024, (void *)tester, 8, NULL, 0);
        vTaskDelay(2000 / portTICK_PERIOD_MS);
    }
}
