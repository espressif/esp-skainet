#pragma once
#include "stdbool.h"
#include "esp_console.h"
#include "argtable3/argtable3.h"

typedef struct 
{
    char mode[32];    // mode
    char noise[32];   // noise
    char snr[32];     // SNR
    int flag;         // update flag
} perf_tester_config_t;


/**
 * @brief Register config command to configure perf tester
 *    config  <Fast/Norm>  <All/None/Pink/Pub>  <All/None/0/5/10>
 *    Perf Tester Config
 *    <Fast/Norm>  Tester mode, Fast: fast check all test cases, Norm: run complete test cases.
 *    <All/None/Pink/Pub>  noise type, All: test all noise data, None: do not add any noise
 *    <All/None/0/5/10>  snr, All: test all snr data, None: do not add any noise, 0/5/10: snr number
 * 
*/
void register_perf_tester_config_cmd(void);

/**
 * @brief Register start command to trigger perf tester
 * 
 * @param start_func  callback function to trigger perf tester
 *                    typedef int (*esp_console_cmd_func_t)(int argc, char **argv);
*/
void register_perf_tester_start_cmd(esp_console_cmd_func_t start_func);


/**
 * @brief Get perf tester config
 * 
 * @return perf tester config 
*/
perf_tester_config_t* get_perf_tester_config(void);

/**
 * Check noise type of filename
 * 
 * @param filename  filename
 * @param noise     noise type
 * 
 * @return true if match, false if not match
*/
bool check_noise(const char *filename, const char *noise);


/**
 * Check SNR of filename
 * 
 * @param filename  filename
 * @param snr       SNR number or 'all', 'none'
 * 
 * @return true if match, false if not match
*/
bool check_snr(const char *filename, const char *snr);