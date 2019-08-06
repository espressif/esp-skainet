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

#ifndef __IM501_SPI_H__
#define __IM501_SPI_H__

typedef enum {
    FILE_IRAM0_FM,
    FILE_DRAM0_FM,
    FILE_DRAM1_FM,
    NUM_FLASH_FILES   /* NUM_FLASH_FILES is always the last one defined */
} FILE_INDEX_T;

int initial_im501();
void im501_int(void* arg);
int im501_fw_loaded();
int im501_spi_read_reg(uint8_t reg, uint8_t* val);
int im501_spi_write_reg(uint8_t reg, uint8_t val);
int im501_spi_write_dram(uint32_t addr, uint8_t* pdata);
int im501_spi_read_dram(uint32_t addr, void* pdata);
int im501_spi_burst_read_dram(uint32_t addr, uint8_t* rxbuf, size_t len);
int im501_spi_write_dram_2byte(uint32_t addr, uint16_t pdata);
void test_channel_register(void);
int set_volume_source(uint8_t channel, uint8_t source);

int request_start_voice_buf_trans(void);
int request_stop_voice_buf_trans(void);
int request_enter_psm(void);

/**
 * codec2im501_pdm_clki_set - external function to set/unset the pdm_clki to im501
 * @set_flag: the flag to set or unset the pdm_clki.
 *            true - set the clk, false - unset the clk.
 * @pdm_clki_rate: the demanded pdm_clki rate. 2MHz for DSP normal mode.
 *                 this parameter is not used when unset the pdm_clki.
 * Return: zero for success set/unset, or non-zero for failure.
 */
extern int codec2im501_pdm_clki_set(bool set_flag, uint32_t pdm_clki_rate);

#endif /* __IM501_SPI_H__ */

