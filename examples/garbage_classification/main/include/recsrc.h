// #pragma once


/**
 * @brief Microphone source (read from the I2S codec on a LyraT board) task fuction
 *
 * @param arg Pointer to an src_cfg_t struct
*/
#define UART_RECORD 0
void recsrcTask(void *arg);
void agc_task(void *parameters);
