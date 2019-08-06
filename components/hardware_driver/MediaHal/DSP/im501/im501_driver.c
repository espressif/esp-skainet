/*
 * ESPRESSIF MIT License
 *
 * Copyright (c) 2018 <ESPRESSIF SYSTEMS (SHANGHAI) PTE LTD>
 *
 * Permission is hereby granted for use on all ESPRESSIF SYSTEMS products, in which case,
 * it is free of charge, to any person obtaining a copy of this software and associated
 * documentation files (the "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the Software is furnished
 * to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all copies or
 * substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
 * COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 * IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 */
#include "userconfig.h"

#if defined CONFIG_ESP_LYRATD_FT_V1_0_BOARD || defined CONFIG_ESP_LYRATD_FT_DOSS_V1_0_BOARD

#include <stdio.h>
#include <string.h>
#include <sys/time.h>   // gettimeofday
#include "esp_log.h"
#include "esp_partition.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "freertos/queue.h"
#include "freertos/timers.h"

#include "im501_i2c_driver.h"
#include "userconfig.h"
#include "DspI2c.h"
#include "EspAudioAlloc.h"
#include "driver/gpio.h"

#define IM501_TAG                   "IM501_DRIVER"
#define TIME_IM501                  500
#define DSP_IRQ_DETECT_PIN_SEL      ((1ULL)<<DSP_IRQ_DETECT_PIN)


#define IM501_ASSERT(a, format, b, ...) \
    if ((a) != 0) { \
        ESP_LOGE(IM501_TAG, format, ##__VA_ARGS__); \
        return b;\
    }
#define IM501_CHECK_NULL(a, format, b, ...) \
    if ((a) == NULL) { \
        ESP_LOGE(IM501_TAG, format, ##__VA_ARGS__); \
        return b;\
    }

#define IM501_LOG(fmt, ...)   ESP_LOGI(IM501_TAG, fmt, ##__VA_ARGS__)

static xTimerHandle TimerIm501;

static void im501_timer_cb(TimerHandle_t xTimer)
{
    int res;
    res = gpio_get_level(DSP_IRQ_DETECT_PIN);
}

static int im501_timer_init()
{
    TimerIm501 = xTimerCreate("im501_timer0", TIME_IM501 / portTICK_RATE_MS, pdFALSE, (void *) 0, im501_timer_cb);

    if (TimerIm501 == NULL) {
        ESP_LOGE(IM501_TAG, "im501_timer create err\n");
        return -1;
    }

    xTimerStart(TimerIm501, TIME_IM501 / portTICK_RATE_MS);

    return 0;
}
static void im501_timer_delete()
{
    xTimerDelete(TimerIm501, TIME_IM501 / portTICK_RATE_MS);
    TimerIm501 = NULL;
}

static void IRAM_ATTR IM501IntrHandler(void *arg)
{
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;

    xTimerResetFromISR(TimerIm501, &xHigherPriorityTaskWoken);

    if (xHigherPriorityTaskWoken != pdFALSE) {
        portYIELD_FROM_ISR();
    }

}

int im501_intr_init()
{
    int res = 0;
    gpio_config_t  io_conf;
    memset(&io_conf, 0, sizeof(io_conf));
    im501_timer_init();
#ifdef IDF_3_0
    io_conf.intr_type = GPIO_INTR_POSEDGE;
#else
    io_conf.intr_type = GPIO_PIN_INTR_POSEDGE;
#endif
    io_conf.pin_bit_mask = DSP_IRQ_DETECT_PIN_SEL;
    io_conf.mode = GPIO_MODE_INPUT;
    io_conf.pull_down_en = 0;
    io_conf.pull_up_en = 0;
    res |= gpio_config(&io_conf);
    res |= gpio_isr_handler_add(DSP_IRQ_DETECT_PIN, IM501IntrHandler, NULL);

    return res;
}

esp_partition_t *im501_partition_init(int subType)
{
    esp_partition_t *DspBinPartition = NULL;
    DspBinPartition = (esp_partition_t *)esp_partition_find_first(ESP_PARTITION_TYPE_DATA, subType, NULL);

    if (NULL == DspBinPartition) {
        ESP_LOGE(IM501_TAG, "Can not find dsp-partition");
        return NULL;
    }

    ESP_LOGI("DspPartition", "%d: type[%x]", __LINE__, DspBinPartition->type);
    ESP_LOGI("DspPartition", "%d: subtype[%x]", __LINE__, DspBinPartition->subtype);
    ESP_LOGI("DspPartition", "%d: address:0x%x", __LINE__, DspBinPartition->address);
    ESP_LOGI("DspPartition", "%d: size:0x%x", __LINE__, DspBinPartition->size);
    ESP_LOGI("DspPartition", "%d: labe:%s", __LINE__,  DspBinPartition->label);

    return DspBinPartition;
}

/*-----------------

    IM501 - I2C

-------------------*/

#define CONTAIN_SHA

typedef enum {
    FILE_IRAM0_FM,
    FILE_DRAM0_FM,
    FILE_DRAM1_FM,
    NUM_FLASH_FILES   /* NUM_FLASH_FILES is always the last one defined */
} FILE_INDEX_T;

int im501_8byte_swap(uint8_t *rxbuf, uint32_t len);

int im501_get_fw_data_info(const uint8_t *pdata, uint16_t data_len, uint32_t *mem_addr, uint16_t *mem_type, uint32_t *mem_size);
void im501_fill_remain_buffer(const uint8_t *pdata, uint16_t offset, uint16_t fill_len, load_dsp_fw *remain_info);
int im501_load_fw_from_new_struct(const uint8_t *pdata, uint16_t data_len, load_dsp_fw *remain_info,
                                  uint8_t cur_status, uint8_t *new_status, uint16_t f_load, uint8_t last_one);
int im501_load_fw_one_block_from_buffer(const uint8_t *pdata, uint16_t fill_len, load_dsp_fw *remain_info, uint16_t f_load);
int im501_load_fw_data_block(const uint8_t *pdata, uint16_t load_len, load_dsp_fw *remain_info, uint16_t f_load);
int im501_load_fw_from_cut_data(const uint8_t *pdata, uint16_t data_len, load_dsp_fw *remain_info,
                                uint8_t cur_status, uint8_t *new_status, uint16_t f_load, uint8_t last_one);
int im501_load_fw_from_cut_struct(const uint8_t *pdata, uint16_t data_len, load_dsp_fw *remain_info,
                                  uint8_t cur_status, uint8_t *new_status, uint16_t f_load, uint8_t last_one);
int im501_load_mem1(uint16_t file_index, uint16_t f_load, uint16_t f_check);

int im501_8byte_swap(uint8_t *rxbuf, uint32_t len)
{
    uint8_t local_buf[8];
    uint32_t i;

    for (i = 0; i < len / 8 * 8; i += 8) {
        memcpy(local_buf, rxbuf, 8);

        rxbuf[i + 0] = local_buf[7];
        rxbuf[i + 1] = local_buf[6];
        rxbuf[i + 2] = local_buf[5];
        rxbuf[i + 3] = local_buf[4];
        rxbuf[i + 4] = local_buf[3];
        rxbuf[i + 5] = local_buf[2];
        rxbuf[i + 6] = local_buf[1];
        rxbuf[i + 7] = local_buf[0];
    }

    return SUCCESS;
}

int im501_get_fw_data_info(const uint8_t *pdata, uint16_t data_len, uint32_t *mem_addr, uint16_t *mem_type, uint32_t *mem_size)
{
    int i = 0;

    *mem_addr = 0;
    *mem_addr = (uint32_t)((uint32_t)(pdata[i]) & 0xFF) | (uint32_t)((uint32_t)(pdata[i + 1] & 0xFF) << 8) |
                (uint32_t)((uint32_t)(pdata[i + 2] & 0xFF) << 16) | (uint32_t)((uint32_t)(pdata[i + 3] & 0xFF) << 24);

    i += 4;
    *mem_type = (uint32_t)(pdata[i] & 0xFF) | (uint32_t)((pdata[i + 1] & 0xFF) << 8);

    i += 2;
    *mem_size = 0;
    *mem_size = (uint32_t)((uint32_t)(pdata[i]) & 0xFF) | (uint32_t)((uint32_t)(pdata[i + 1] & 0xFF) << 8) |
                (uint32_t)((uint32_t)(pdata[i + 2] & 0xFF) << 16) | (uint32_t)((uint32_t)(pdata[i + 3] & 0xFF) << 24);
    /*
        ESP_LOGI(IM501_TAG, "get new structure address=%lx, size=%lx\n", *mem_addr, *mem_size);
    */
    return SUCCESS;
}

void im501_fill_remain_buffer(const uint8_t *pdata, uint16_t offset, uint16_t fill_len, load_dsp_fw *remain_info)
{
    uint16_t i;

    for (i = 0; i < fill_len; i++) {
        remain_info->remain_buffer[i + offset] = pdata[i];
    }

    remain_info->remain_buffer_len = fill_len + offset;

    return;
}

int im501_load_fw_from_new_struct(const uint8_t *pdata, uint16_t data_len, load_dsp_fw *remain_info,
                                  uint8_t cur_status, uint8_t *new_status, uint16_t f_load, uint8_t last_one)
{
    uint32_t            mem_addr;
    uint16_t            mem_type;
    uint32_t            mem_size;
    uint16_t            left_data_len;
    const uint8_t  *pdata_ptr = NULL;
    uint16_t            j;
    uint16_t            ret = SUCCESS;
    uint8_t         sha1_len;

#ifdef CONTAIN_SHA
    sha1_len = 40;
#else
    sha1_len = 0;
#endif

    if (pdata == NULL) {
        return E_ERROR;
    }

    if (((data_len + remain_info->remain_buffer_len) == 0) && (last_one == 1)) {
        return SUCCESS;
    }

    if (((data_len + remain_info->remain_buffer_len) < (10 + sha1_len)) && (last_one == 1)) { /*struct size is 4 + 2 + 4, the last block does not contains complete struct, it means bad data.*/
        return E_ERROR;
    }

    left_data_len = data_len;
    pdata_ptr = pdata;

    while (1) {
        if (left_data_len < 10) { /*struct size is 4 + 2 + 4*/
            *new_status = LOAD_FW_CUT_STRUCT;
            im501_fill_remain_buffer(pdata_ptr, 0, left_data_len, remain_info);
            ret = SUCCESS;
            break;
        }

        im501_get_fw_data_info(pdata_ptr, left_data_len, &mem_addr, &mem_type, &mem_size);

        pdata_ptr += 10;
        left_data_len -= 10;

        if (left_data_len < (sha1_len + mem_size)) { /*struct size is 4 + 2 + 4, SHA1 data is 40 bytes, others are data.*/
            if (left_data_len > mem_size) { /*contain all data, but SHA data is cut*/
                ret = im501_download_mem_data(pdata_ptr, mem_addr, mem_type, mem_size, f_load);

                if (ret != SUCCESS) {
                    break;
                }

                *new_status = LOAD_FW_CUT_SHA;
                remain_info->remain_buffer_len = left_data_len - mem_size;

                ret = SUCCESS;
                break;
            } else { /*data is cut, load data to dsp as much as possible*/
                j = left_data_len / CSR_BLOCKSIZE;
                ret = im501_download_mem_data(pdata_ptr, mem_addr, mem_type, CSR_BLOCKSIZE * j, f_load);

                if (ret != SUCCESS) {
                    break;
                }

                *new_status = LOAD_FW_CUT_DATA;
                left_data_len = left_data_len - j * CSR_BLOCKSIZE;
                pdata_ptr += j * CSR_BLOCKSIZE;
                im501_fill_remain_buffer(pdata_ptr, 0, left_data_len, remain_info);
                remain_info->remain_data_addr = mem_addr + j * CSR_BLOCKSIZE;
                remain_info->remain_data_len = mem_size - j * CSR_BLOCKSIZE;
                remain_info->remain_data_type = mem_type;

                ret = SUCCESS;
                break;
            }
        } else { /*whole data strucure is in this block, load it to dsp and deal the nexit struture*/
            ret = im501_download_mem_data(pdata_ptr, mem_addr, mem_type, mem_size, f_load);

            if (ret != SUCCESS) {
                break;
            }

            left_data_len -= (mem_size + sha1_len);
            pdata_ptr += (mem_size + sha1_len);

        }
    }

    return ret;
}

int im501_load_fw_one_block_from_buffer(const uint8_t *pdata, uint16_t fill_len, load_dsp_fw *remain_info, uint16_t f_load)
{
    uint16_t ret = SUCCESS;
    uint16_t i;
    uint16_t data_len;

    data_len = fill_len + remain_info->remain_buffer_len;

    if ((pdata == NULL) || (remain_info == NULL)) {
        return E_ERROR;
    }

    if (data_len == 0) {
        return E_ERROR;
    }

    for (i = 0; i < fill_len; i++) {
        remain_info->remain_buffer[remain_info->remain_buffer_len + i] = pdata[i];
    }

    ret = im501_download_mem_data(remain_info->remain_buffer, remain_info->remain_data_addr, remain_info->remain_data_type, data_len, f_load);

    if (ret != SUCCESS) {
        return E_ERROR;
    }

    remain_info->remain_buffer_len = 0;

    remain_info->remain_data_addr += data_len;
    remain_info->remain_data_len -= data_len;

    return ret;
}

int im501_load_fw_data_block(const uint8_t *pdata, uint16_t load_len, load_dsp_fw *remain_info, uint16_t f_load)
{
    uint16_t    ret = SUCCESS;

    ret = im501_download_mem_data(pdata, remain_info->remain_data_addr, remain_info->remain_data_type, load_len, f_load);
    remain_info->remain_data_addr += load_len;
    remain_info->remain_data_len -= load_len;

    return ret;
}

int im501_load_fw_from_cut_data(const uint8_t *pdata, uint16_t data_len, load_dsp_fw *remain_info,
                                uint8_t cur_status, uint8_t *new_status, uint16_t f_load, uint8_t last_one)
{
    uint16_t            left_data_len;
    const uint8_t  *pdata_ptr = NULL;
    uint16_t            j;
    uint16_t            ret = SUCCESS;
    uint16_t            temp_len;


    if (pdata == NULL) {
        return E_ERROR;
    }

    if (((data_len + remain_info->remain_buffer_len) < (remain_info->remain_data_len)) && (last_one == 1)) { /*data is not enough.*/
        return E_ERROR;
    }

    pdata_ptr = pdata;
    left_data_len = data_len;

    if ((left_data_len + remain_info->remain_buffer_len) <= remain_info->remain_data_len) {
        if ((left_data_len + remain_info->remain_buffer_len) > CSR_BLOCKSIZE) {
            /*deal with the data remain in last loading*/
            temp_len = CSR_BLOCKSIZE - remain_info->remain_buffer_len;

            ret = im501_load_fw_one_block_from_buffer(pdata_ptr, temp_len, remain_info, f_load);

            if (ret != SUCCESS) {
                return E_ERROR;
            }

            pdata_ptr += temp_len;
            left_data_len -= temp_len;

            /*deal with the other data*/
            j = left_data_len / CSR_BLOCKSIZE;
            temp_len = j * CSR_BLOCKSIZE;
            ret = im501_load_fw_data_block(pdata_ptr, temp_len, remain_info, f_load);

            if (ret != SUCCESS) {
                return E_ERROR;
            }

            left_data_len -= temp_len;
            pdata_ptr += temp_len;

            *new_status = LOAD_FW_CUT_DATA;
            im501_fill_remain_buffer(pdata_ptr, 0, left_data_len, remain_info);

            ret = SUCCESS;
        } else { /*append to buffer */
            *new_status = LOAD_FW_CUT_DATA;
            im501_fill_remain_buffer(pdata_ptr, remain_info->remain_buffer_len, left_data_len, remain_info);

            pdata_ptr += left_data_len;
            left_data_len = 0;
            ret = SUCCESS;
        }
    } else {
        /*deal with the data remain in last loading*/
        if (remain_info->remain_data_len > CSR_BLOCKSIZE) {
            temp_len = CSR_BLOCKSIZE - remain_info->remain_buffer_len;
        } else {
            temp_len = remain_info->remain_data_len - remain_info->remain_buffer_len;
        }

        ret = im501_load_fw_one_block_from_buffer(pdata_ptr, temp_len, remain_info, f_load);

        if (ret != SUCCESS) {
            return E_ERROR;
        }

        pdata_ptr += temp_len;
        left_data_len -= temp_len;

        /*deal with the other data*/
        temp_len = remain_info->remain_data_len;
        ret = im501_load_fw_data_block(pdata_ptr, temp_len, remain_info, f_load);

        if (ret != SUCCESS) {
            return E_ERROR;
        }

        left_data_len -= temp_len;
        pdata_ptr += temp_len;

#ifdef CONTAIN_SHA

        if (left_data_len >= 40) { /*deal with SHA*/
            left_data_len -= 40;
            pdata_ptr += 40;

            *new_status = LOAD_FW_START;
            ret = im501_load_fw_from_new_struct(pdata_ptr, left_data_len, remain_info, *new_status, new_status, f_load, last_one);
        } else {
            *new_status = LOAD_FW_CUT_SHA;
            remain_info->remain_buffer_len = left_data_len;

            ret = SUCCESS;
        }

#else
        *new_status = LOAD_FW_START;
        ret = im501_load_fw_from_new_struct(pdata_ptr, left_data_len, remain_info, *new_status, new_status, f_load, last_one);
#endif
    }

    return ret;
}

int im501_load_fw_from_cut_struct(const uint8_t *pdata, uint16_t data_len, load_dsp_fw *remain_info,
                                  uint8_t cur_status, uint8_t *new_status, uint16_t f_load, uint8_t last_one)
{
    uint32_t            mem_addr;
    uint16_t            mem_type;
    uint32_t            mem_size;
    uint16_t            left_data_len;
    const uint8_t  *pdata_ptr = NULL;
    uint16_t            i;
    uint16_t            ret = SUCCESS;
    uint16_t            temp_len;

    if (pdata == NULL) {
        return E_ERROR;
    }

    if (((data_len + remain_info->remain_buffer_len) <= (remain_info->remain_data_len)) && (last_one == 1)) { /*data is not enough.*/
        return E_ERROR;
    }

    left_data_len = data_len;
    pdata_ptr = pdata;

    /*deal with the structure and first data block remain in last loading*/
    temp_len = 14 - (remain_info->remain_buffer_len); /*4 + 2 + 4, add 4 bytes extra data*/

    for (i = 0; i < temp_len; i++) {
        remain_info->remain_buffer[(remain_info->remain_buffer_len) + i] = pdata_ptr[i];
    }

    remain_info->remain_buffer_len = 0;

    im501_get_fw_data_info(remain_info->remain_buffer, 14, &mem_addr, &mem_type, &mem_size);
    ret = im501_download_mem_data(remain_info->remain_buffer + 10, mem_addr, mem_type, 4, f_load);

    if (ret != SUCCESS) {
        return E_ERROR;
    }

    pdata_ptr += temp_len;
    left_data_len -= temp_len;

    if (mem_size != 4) {
        remain_info->remain_data_addr   = mem_addr + 4;
        remain_info->remain_data_len    = mem_size - 4;
        remain_info->remain_data_type   = mem_type;
        remain_info->remain_buffer_len  = 0;

        ret = im501_load_fw_from_cut_data(pdata_ptr, left_data_len, remain_info, LOAD_FW_CUT_DATA, new_status, f_load, last_one);
    } else {
#ifdef CONTAIN_SHA

        if (left_data_len >= 40) {
            pdata_ptr += 40;
            left_data_len -= 40;
        } else {
            *new_status = LOAD_FW_CUT_SHA;
            remain_info->remain_buffer_len = left_data_len;

            return SUCCESS;
        }

#endif
        ret = im501_load_fw_from_new_struct(pdata_ptr, left_data_len, remain_info, LOAD_FW_START, new_status, f_load, last_one);
    }

    return ret;
}

#define INTERNAL_SIZE (0xc00)


int im501_load_mem1(uint16_t file_index, uint16_t f_load, uint16_t f_check)
{
    uint32_t            iii = 0;
    uint16_t         temp_len;
    uint8_t           *lSource;
    uint8_t         status      = LOAD_FW_START;
    uint8_t         new_status  = LOAD_FW_UNDEFINED;
    uint16_t            ret = SUCCESS;
    uint8_t         last_block = 0;
    load_dsp_fw     remain_info;
    esp_partition_t *DspBin = NULL;
    int index = 0, ret_len = 0, offset = 0;
    index = 0x20 + file_index;

    // partition table
    DspBin = im501_partition_init(index);
    IM501_CHECK_NULL(DspBin, "partition init", -1);
    lSource = EspAudioAllocInner(1, INTERNAL_SIZE);
    IM501_CHECK_NULL(lSource, "EspAudioAllocInner", -1);

    for (iii = 0; iii < 100000; iii++) { /* CSR file system return max of 0xc00, if more than 0xc00 */

        if (offset + INTERNAL_SIZE < DspBin->size) {
            ret_len = INTERNAL_SIZE;
        } else {
            ret_len = DspBin->size - offset;
            last_block = 1;
        }

        if (esp_partition_read(DspBin, offset, lSource, ret_len) != 0) {
            ESP_LOGE(IM501_TAG, "in %s, line: %d", __func__, __LINE__);
            goto END;
        }

        offset += ret_len;

        switch (status) {
            case LOAD_FW_START: /*1. from struct start, status == LOAD_FW_START*/
                ret = im501_load_fw_from_new_struct((const uint8_t *)lSource, ret_len, &remain_info, status, &new_status, f_load, last_block);

                if (ret != SUCCESS) {
                    break;
                }

                status = new_status;
                break;

#ifdef CONTAIN_SHA

            case LOAD_FW_CUT_SHA: /*2. deal with unfinished SHA loading, status == LOAD_FW_CUT_SHA*/
                temp_len = ret_len;

                if ((temp_len + remain_info.remain_buffer_len) < 40) {
                    status = LOAD_FW_CUT_SHA;
                    remain_info.remain_buffer_len += temp_len;
                    lSource += temp_len;
                    temp_len = 0;
                    break;
                } else {
                    lSource += (40 - remain_info.remain_buffer_len);
                    temp_len -= (40 - remain_info.remain_buffer_len);
                    status = LOAD_FW_START;
                    remain_info.remain_buffer_len = 0;

                    if (temp_len > 0) {
                        ret = im501_load_fw_from_new_struct((const uint8_t *)lSource, temp_len, &remain_info, status, &new_status, f_load, last_block);
                    } else { /* ==40 */
                        break;
                    }
                }

                if (ret != SUCCESS) {
                    break;
                }

                status = new_status;

                break;
#endif

            case LOAD_FW_CUT_STRUCT: /*3. deal with incomplete statuct and new loaded data, status == LOAD_FW_CUT_STRUCT*/
                ret = im501_load_fw_from_cut_struct((const uint8_t *)lSource, ret_len, &remain_info, status, &new_status, f_load, last_block);

                if (ret != SUCCESS) {
                    break;
                }

                status = new_status;
                break;

            case LOAD_FW_CUT_DATA: /*4. deal with unfinished data loading, status == LOAD_FW_CUT_DATA*/
                ret = im501_load_fw_from_cut_data((const uint8_t *)lSource, ret_len, &remain_info, status, &new_status, f_load, last_block);

                if (ret != SUCCESS) {
                    break;
                }

                status = new_status;
                break;

            case LOAD_FW_COMPLETE: /*5. all data loaded, status == LOAD_FW_COMPLETE*/
                break;

            case LOAD_FW_UNDEFINED: /*6. illegal status*/
            default:
                break;
        }


        if (last_block == 1) {
            if (remain_info.remain_buffer_len != 0) {
                if (status == LOAD_FW_CUT_DATA) {
                    ret = im501_download_mem_data(remain_info.remain_buffer, remain_info.remain_data_addr, remain_info.remain_data_type, remain_info.remain_data_len, f_load);

                    if (ret != SUCCESS) { /*error*/
                        ESP_LOGE(IM501_TAG, "in %s, line: %d", __func__, __LINE__);
                    }

                    break;
                } else {
                    /*if go here, should be wrong!*/
                    ESP_LOGE(IM501_TAG, "in %s, line: %d", __func__, __LINE__);
                    break;
                }
            }

            break;
        }
    }

END:
    free(lSource);
    return 0;
}


/**
 * @brief Initialize the IM501 DSP
 *
 * @param reflash  a flag indicating whether to reflash the DSP flash or not,
 *                  three different binaries must be flashed, they are Iram0.bin, Dram0.bin and Dram1.bin
 *                  the three binaries are stored in different flash areas indicated by the partition table in 'partitions_esp_audio.csv'.
 *
 * @return
 *     - (-1) Failed
 *     - ( 0) Succeed
 */
int im501_load_mem(uint16_t file_index, uint16_t f_load, uint16_t f_check)
{
    uint32_t            iii = 0;
    uint16_t            i, j, a, b, c, ret_len;
    uint8_t            *lSource;
    uint16_t            ret = SUCCESS;
    uint8_t             last_block = 0;
    uint16_t            mem_type = 0;
    uint32_t            mem_base_addr = 0x10000000;
    uint32_t            address;
    uint16_t            offset = 0;
    uint8_t            *local_buf = NULL;
    uint8_t            *buffer = NULL;

    esp_partition_t *DspBin = NULL;
    int index = 0, partitionOffset = 0;
    index = 0x20 + file_index;

    if (file_index == FILE_IRAM0_FM) {
        mem_type = 0;
        mem_base_addr = 0x10000000;
    } else {
        mem_type = 1;

        if (file_index == FILE_DRAM0_FM) {
            mem_base_addr = 0x0ffc0000;
        } else {
            mem_base_addr = 0x0ffe0000;
        }
    }

    local_buf = EspAudioAlloc(1, CSR_BLOCKSIZE + 4);
    IM501_CHECK_NULL(local_buf, "EspAudioAlloc", -1);
    buffer = EspAudioAlloc(1, CSR_BLOCKSIZE + 4);

    // partition table
    DspBin = im501_partition_init(index);
    lSource = EspAudioAllocInner(1, INTERNAL_SIZE);

    if ((buffer == NULL) || (DspBin == NULL) || (lSource == NULL)) {
        ESP_LOGE(IM501_TAG, "in %s, line: %d out of memory", __func__, __LINE__);
        goto END;
    }

    struct timeval pre, now;

    int partitionTime = 0, loadTime = 0;

    for (iii = 0; iii < 100000; iii++) { /* CSR file system return max of 0xc00, if more than 0xc00 */

        gettimeofday(&pre, NULL);

        if (partitionOffset + INTERNAL_SIZE < DspBin->size) {
            ret_len = INTERNAL_SIZE;
        } else {
            ret_len = DspBin->size - partitionOffset;
            last_block = 1;
        }

        if (esp_partition_read(DspBin, partitionOffset, lSource, ret_len) != 0) {
            ESP_LOGE(IM501_TAG, "in %s, line: %d", __func__, __LINE__);
            goto END;
        }

        partitionOffset += ret_len;
        gettimeofday(&now, NULL);

        a = ret_len / CSR_BLOCKSIZE;
        b = ret_len % CSR_BLOCKSIZE;
        c = CSR_BLOCKSIZE / 4;
        address = mem_base_addr + iii * 0x0C00;
        offset = 0;
        gettimeofday(&pre, NULL);

        if (ret_len != 0) {
            for (j = 0; j < a; j++) {
                /*
                                ESP_LOGI(IM501_TAG, "will download data to %lx, write %x bytes , index=%d\n", address + offset, CSR_BLOCKSIZE, j);
                */
                memset(buffer, 0, (CSR_BLOCKSIZE + 4));

                if (offset == 0xbc0) {
                    for (i = 0; i < CSR_BLOCKSIZE; i++) {
                        buffer[i] = lSource[offset + i];
                    }
                } else {
                    memcpy(buffer, lSource + offset, CSR_BLOCKSIZE);
                }

                im501_8byte_swap(buffer, CSR_BLOCKSIZE);

                ret = im501_download_mem_data(buffer/*lSource + offset*/, address + offset, mem_type, CSR_BLOCKSIZE, f_load);

                if (ret != SUCCESS) { /* error occurs */
                    break;
                }

                if (f_check == TRUE) {
                    memset(local_buf, 0, CSR_BLOCKSIZE + 4);

                    for (i = 0; i < c; i++) {
                        ret = im501_i2c_mem_read(address + offset + i * 4, local_buf + i * 4);
                    }

                    for (i = 0; i < CSR_BLOCKSIZE; i++) {
                        if (local_buf[i] != buffer[i]) {
                            ESP_LOGE(IM501_TAG, "1 fw read %x vs write %x @ %x address:0x%x i:0x%x offset:0x%x\n",
                                     local_buf[i], buffer[i + offset], address + i + offset, address, i, offset);
                            goto END;
                        }
                    }
                }

                offset += CSR_BLOCKSIZE;
            }

            if (j != a) { /* error occurs */
                break;
            }

            if (b != 0) {
                memset(buffer, 0, CSR_BLOCKSIZE + 4);
                memcpy(buffer, lSource + offset, b);
                im501_8byte_swap(buffer, b);

                ret = im501_download_mem_data(buffer/*lSource + offset*/, address + offset, mem_type, b, f_load);

                if (ret != SUCCESS) { /* error occurs */
                    break;
                }

                if (f_check == TRUE) {
                    memset(local_buf, 0, CSR_BLOCKSIZE + 4);
                    c = b / 4;

                    for (i = 0; i < c; i++) {
                        ret = im501_i2c_mem_read(address + offset + i * 4, local_buf + i * 4);
                    }

                    for (i = 0; i < b; i++) {
                        if (local_buf[i] != buffer[i]) {
                            ESP_LOGE(IM501_TAG, "2 fw read %x vs write %x @ %x\n", local_buf[i], lSource[i + offset], address + i + offset);
                            goto END;
                        }
                    }
                }
            }

            gettimeofday(&now, NULL);
            loadTime = (now.tv_sec - pre.tv_sec) * 1000 + (now.tv_usec - pre.tv_usec) / 1000;


            ESP_LOGW(IM501_TAG, "Offset: %d, time %d, time %d", partitionOffset, partitionTime, loadTime);
        }


        if (last_block == 1) {
            break;
        }
    }

END:

    free(local_buf);
    free(buffer);
    free(lSource);

    return 0;
}

int im501_load_firmware(void)
{
//    uint32_t pio_ret = 0;

//  /* Set up task handler */
//  //SIMPLE.task.handler = profile_handler;


//  MessagePioTask(&SIMPLE.task);  /* Register for button press messages */
//  /* on i-voice, top button = 4, bottom button = 2 */
//  /* on 1645: vol+ =0x800, vol- =0x1000, play/stop=0x2000, fwd=0x4000, back=0x8000, aux1=0x400, aux2=2, bluemedia=1 */
//  /* first parameter=2 configures bottom button on i-Voice or aux2 on 1645 */
//  /* set first parameter to 0xffff to activate ALL possible pio buttons for testing new hardware */

//#ifdef ADK2P0_COMPATIBLE
//  /* initialize the audio library */
//  AudioLibraryInit();
//#endif

    im501_intr_init();

    DspI2cInit();
    im501_switch_i2c();
    im501_write_and_check(0x0F, 0x07);//IM501_I2C_DSP_CTRL_REG
    im501_write_and_check(0x0F, 0x05);//IM501_I2C_DSP_CTRL_REG

    vTaskDelay(10 / portTICK_RATE_MS);

    im501_load_mem(FILE_IRAM0_FM, TRUE, FALSE);
    im501_load_mem(FILE_DRAM0_FM, TRUE, FALSE);
    im501_load_mem(FILE_DRAM1_FM, TRUE, FALSE);

    vTaskDelay(10 / portTICK_RATE_MS);


    im501_write_and_check(0x0F, 0x04);


    int value = 0;

    if (SUCCESS != im501_i2c_reg_read(0x0FFFFF32, (uint8_t *)&value, 1)) {
        ESP_LOGE(IM501_TAG, "in %s line %d", __func__, __LINE__);
        return E_ERROR;
    }

    ESP_LOGI(IM501_TAG, "value %d", value);

    /*check DSP working or not*/
    check_dsp_status();


//    MessageLoop();

    /* Never get here! */
    return 0;
}





#endif
