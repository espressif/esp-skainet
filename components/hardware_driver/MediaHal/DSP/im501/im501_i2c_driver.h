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

#ifndef _IM501_I2C_DRIVER_H_
#define _IM501_I2C_DRIVER_H_

#define SUCCESS                         0u
#define E_ERROR							1u

#define TRUE   1
#define FALSE  0

#define CSR_BLOCKSIZE					0x40

typedef struct load_dsp_fw_t {
    uint8_t remain_buffer[80];
    uint16_t remain_buffer_len;
    uint32_t remain_data_addr;
    uint32_t remain_data_len;
    uint16_t remain_data_type;
} load_dsp_fw;

typedef enum {
    LOAD_FW_START,
    LOAD_FW_CUT_SHA,
    LOAD_FW_CUT_STRUCT,
    LOAD_FW_CUT_DATA,
    LOAD_FW_COMPLETE,
    LOAD_FW_UNDEFINED
} LOAD_FW_STATUS_T;


uint32_t im501_get_version(void);
void im501_clock_delay(uint32_t ms);

int im501_i2c_reg_read(uint8_t reg_no, uint8_t* data, uint16_t len);
int im501_i2c_reg_write(uint8_t reg, uint32_t value, uint16_t len);
int im501_i2c_mem_read(uint32_t mem_addr, uint8_t* pdata);
int im501_i2c_mem_write(uint32_t mem_addr, const uint8_t* pdata);
int im501_i2c_mem_burst_write(uint32_t mem_addr, const uint8_t* pdata, uint16_t len);

int im501_download_mem_data_fast(const uint8_t* pdata, uint32_t mem_addr, uint32_t mem_size, uint8_t unit);
int im501_download_mem_data_slow(const uint8_t* pdata, uint32_t mem_addr, uint32_t mem_size, uint8_t unit);
int im501_download_mem_data(const uint8_t* pdata_ptr, uint32_t mem_addr, uint16_t mem_type, uint32_t mem_size, uint16_t f_load);

int im501_switch_i2c(void);
int im501_write_and_check(uint8_t reg, uint8_t val);
int check_dsp_status(void);

int im501_pdm_clki_set(uint16_t set_flag, uint32_t pdm_clki_rate);
//int im501_parse_to_host_command(to_host_cmd cmd);
//int im501_send_to_dsp_command(to_501_cmd cmd);
int im501_send_message_to_dsp(uint8_t cmd_index, uint32_t para);

int request_enter_psm(void);
int request_enter_normal(void);
int request_enter_hw_bypass(void);

#endif /* _IM501_I2C_DRIVER_H_ */
