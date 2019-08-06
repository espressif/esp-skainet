/* 
   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "esp_system.h"
#include "driver/i2s.h"
#include "ringbuf.h"
#include "recsrc.h"

extern struct RingBuf *rec_rb;

#define REC_FRAME_BYTES 960

void recsrcTask(void *arg)
{
    int16_t *rsp_in = malloc(REC_FRAME_BYTES * 2);
    int16_t *rsp_out = malloc(REC_FRAME_BYTES);
    size_t bytes_read;
    while (1) {
        // i2s_read_bytes(I2S_NUM_1, rsp_in, 2 * REC_FRAME_BYTES, portMAX_DELAY);
        i2s_read(I2S_NUM_1, rsp_in, 2 * REC_FRAME_BYTES, &bytes_read, portMAX_DELAY);

        for (int i = 0; i < REC_FRAME_BYTES / 2; i++) {
            rsp_out[i] = rsp_in[2 * i];
        }
        rb_write(rec_rb, (uint8_t *)rsp_out, REC_FRAME_BYTES, portMAX_DELAY);
    }
}