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
#include "esp_log.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "freertos/queue.h"

#include "im501_i2c_driver.h"
#include "DspI2c.h"

#define IM501_TAG "IM501_DRIVER"



#define IM501_VERSION                   0x0100  /* high byte: version, low byte: sub-version */

#define IM501_I2C_ID                    (0xE2)      /* 0xE2 is 7 bits device id */

#define HWBYPASS_MODE                   2
#define NORMAL_MODE                     1
#define POWER_SAVING_MODE               0

#define IM501_I2C_COUNT_REG             0x08
#define IM501_I2C_DSP_CTRL_REG          0x0F
#define IM501_I2C_IRQ_REG               0x10
#define IM501_I2C_SPI_REG               0x12/*bit2 = 1 means I2C mode, default is SPI mode*/

#define IM501_DRAM_BASE                 0x0FFC0000
#define IM501_IRAM_BASE                 0x10000000
#define TO_DSP_FRAMECOUNTER_ADDR        0x0FFFBEFC
#define TO_DSP_CMD_ADDR                 0x0FFFBFF8
#define TO_HOST_CMD_ADDR                0x0FFFBFFC

#define TO_DSP_CMD_REQ_ENTER_PSM        0x3
#define TO_DSP_CMD_REQ_ENTER_NORMAL     0x4
#define TO_DSP_CMD_REQ_ENTER_HW_BYPASS  0x1B


#define TO_HOST_CMD_KEYWORD_DET         0x40
#define TO_HOST_CMD_DATA_BUF_RDY        0x41

#define IM501_I2C_CMD_DM_WR             0x2B /*For burst mode, only can be 2 bytes*/
#define IM501_I2C_CMD_DM_RD             0x27 /*Normal W/R, can be 1,2,4 bytes. */
#define IM501_I2C_CMD_IM_WR             0x0D
#define IM501_I2C_CMD_IM_RD             0x07
#define IM501_I2C_CMD_REG_WR_1          0x48
#define IM501_I2C_CMD_REG_WR_2          0x4A
#define IM501_I2C_CMD_REG_RD            0x46 /*only support one byte read*/
#define IM501_I2C_CMD_DM_WR_BST         0xA8
#define IM501_I2C_CMD_DM_RD_BST         0xA0
#define IM501_I2C_CMD_IM_WR_BST         0x88
#define IM501_I2C_CMD_IM_RD_BST         0x80

#define MEM_TYPE_IRAM                   0
#define MEM_TYPE_DRAM                   1

#define RESET_PIO                       (1 << 2)
#define PDM_CLK_PIO                     (1 << 4)



typedef struct to_host_cmd_t {
    uint32_t  attri_cmdbyte_status;
} to_host_cmd;

typedef struct to_im501_cmd_t {
    uint32_t  attri_cmdbyteext_status;
    uint8_t cmd_byte;
} to_501_cmd;


/*--------------------------------------------------------------------------------------
// To use iM501 API, caller must provide following callback functions

// write register thru I2C, implement i2c transaction as:
// "S id+W reg_no data[0] data[1] data[2] .... data[len-1] P"
// return 0: success, 1: fail
extern int _i2c_reg_write(unsigned char reg_no,unsigned char *data, unsigned char len);
// read register thru I2C, implement i2c transaction as:
// "S id+W reg_no S id+R data_r[0] data_r[1] data_r[2] .... data_r[len-1] P"
// return 0: success, 1: fail
extern int _i2c_reg_read(unsigned char reg_no, unsigned char *data, unsigned char len);
// delay for ready to access im401 register, 100 micro-second delay is enough
extern void _clock_delay(void);
--------------------------------------------------------------------------------------*/

int im501_host_irqstatus = 0;
int im501_dsp_mode_old = -1;



/* get API version, high byte is version, low byte is sub-version */
uint32_t im501_get_version(void)
{
    return IM501_VERSION;
}

void im501_clock_delay(uint32_t ms)
{
    vTaskDelay(ms / portTICK_RATE_MS);
}

int im501_i2c_reg_read(uint8_t reg_no, uint8_t* data, uint16_t len)
{
    uint16_t  test_count2;
    uint8_t   i2c_buf[2];

    i2c_buf[0] = IM501_I2C_CMD_REG_RD;
    i2c_buf[1] = reg_no;

    test_count2 = DspI2cReadReg(IM501_I2C_ID, (uint8_t*)i2c_buf, 2, (uint8_t*)data, len);

    if (test_count2 == 0) {
        return SUCCESS;
    } else {
        /* Panic(); */
        return E_ERROR;
    }
}

int im501_i2c_reg_write(uint8_t reg, uint32_t value, uint16_t len)
{
    uint16_t  test_count2;
    uint8_t   i2c_buf[4];

    if (len == 1) {
        i2c_buf[0] = IM501_I2C_CMD_REG_WR_1;
        i2c_buf[1] = reg;
        i2c_buf[2] = value & 0xff;
    } else if (len == 2) {
        i2c_buf[0] = IM501_I2C_CMD_REG_WR_2;
        i2c_buf[1] = reg;
        i2c_buf[2] = value & 0xff;
        i2c_buf[3] = (value >> 8) & 0xff;
    } else {
        return E_ERROR;
    }

    test_count2 = DspI2cWrite(IM501_I2C_ID, (uint8_t*)i2c_buf, len + 2);

    if (test_count2 == 0) {
        return SUCCESS;  /* success */
    } else {

        /*   Panic(); */
        return E_ERROR;
    }
}

int im501_switch_i2c(void)
{
    uint16_t  err = SUCCESS;
    uint8_t   data;

    err = im501_i2c_reg_read(IM501_I2C_SPI_REG, &data, 1);

    if (err != SUCCESS) {
        return err;
    }

//printf("current SPI/I2C status = %d\n", data);

    if (!(data & 0x04)) { // SPI mode, need switch to I2C
//printf("will set SPI/I2C to I2C\n");
        err = im501_i2c_reg_write(IM501_I2C_SPI_REG, 0x04, 1);

        if (err != SUCCESS) {
            return err;
        }

//printf("set SPI/I2C to I2C success.\n");
    }

    return err;
}

int im501_i2c_mem_read(uint32_t mem_addr, uint8_t* pdata)
{
    uint16_t    err = SUCCESS;
    uint8_t i2c_buf[4];
    uint8_t read_cmd;
    uint16_t    test_count2;

    if ((mem_addr & IM501_IRAM_BASE) == IM501_IRAM_BASE) { /*IRAM read*/
        read_cmd = IM501_I2C_CMD_IM_RD;
    } else {    /*DRAM read*/
        read_cmd = IM501_I2C_CMD_DM_RD;
    }

    /* Write register */
    i2c_buf[0] = read_cmd;
    i2c_buf[1] = mem_addr & 0xFF;
    i2c_buf[2] = (mem_addr >> 8) & 0xFF;
    i2c_buf[3] = (mem_addr >> 16) & 0xFF;

    test_count2 = DspI2cWrite(IM501_I2C_ID, (uint8_t*)i2c_buf, 4);

    if (test_count2 != 0) {
        return E_ERROR;
    }

    /* Read data */
    err = im501_i2c_reg_read(0x0A, (uint8_t*)pdata, 1);

    if (err != SUCCESS) {
        return err;
    }

    err = im501_i2c_reg_read(0x0B, (uint8_t*)(pdata + 1), 1);

    if (err != SUCCESS) {
        return err;
    }

    err = im501_i2c_reg_read(0x0C, (uint8_t*)(pdata + 2), 1);

    if (err != SUCCESS) {
        return err;
    }

    err = im501_i2c_reg_read(0x0D, (uint8_t*)(pdata + 3), 1);

    if (err != SUCCESS) {
        return err;
    }

    return err;
}

int im501_i2c_mem_write(uint32_t mem_addr, const uint8_t* pdata)
{
    uint8_t i2c_buf[8];
    uint16_t    len = 0;
    uint16_t    test_count2;

    memset(i2c_buf, 0, 4);
    i2c_buf[1] = mem_addr & 0xFF;
    i2c_buf[2] = (mem_addr >> 8) & 0xFF;
    i2c_buf[3] = (mem_addr >> 16) & 0xFF;
    i2c_buf[4] = *pdata;
    i2c_buf[5] = *(pdata + 1);

    if ((mem_addr & IM501_IRAM_BASE) == IM501_IRAM_BASE) { /*IRAM write*/
        i2c_buf[0] = IM501_I2C_CMD_IM_WR;
        i2c_buf[6] = *(pdata + 2);
        i2c_buf[7] = *(pdata + 3);
        len = 8;
    } else {    /*DRAM write*/
        i2c_buf[0] = IM501_I2C_CMD_DM_WR;
        len = 6;
    }

    test_count2 = DspI2cWrite(IM501_I2C_ID, (uint8_t*)i2c_buf, len);

    if (test_count2 == 0) {
        return SUCCESS;
    }

    return E_ERROR;
}

int im501_i2c_mem_burst_write(uint32_t mem_addr, const uint8_t* pdata, uint16_t len)
{
    uint16_t    ret = SUCCESS;
    uint8_t     i2c_buf[1];
    uint8_t write_cmd;
    uint16_t    offset = 0;
    uint16_t    test_count2;

    /*burst*/
    if ((mem_addr & IM501_IRAM_BASE) == IM501_IRAM_BASE) { /*IRAM burst write*/
        write_cmd = IM501_I2C_CMD_IM_WR_BST;
        offset = 4;
    } else {    /*DRAM burst write*/
        write_cmd = IM501_I2C_CMD_DM_WR_BST;
        offset = 2;
    }

    ret = im501_i2c_reg_write(IM501_I2C_COUNT_REG, len - offset, 2);

    if (ret != SUCCESS) {
        return ret;
    }

    ret = im501_i2c_mem_write(mem_addr, pdata);

    if (ret != SUCCESS) {
        return ret;
    }

    i2c_buf[0] = write_cmd;
    test_count2 = DspI2cWrite(IM501_I2C_ID, (uint8_t*)i2c_buf, 1);

    if (test_count2 == 0) {
        test_count2 = DspI2cWrite(IM501_I2C_ID, (uint8_t*)(pdata + offset), len - offset);

        if (test_count2 == 0) {
            return SUCCESS;
        }
    }

    return E_ERROR;
}

/* for write and check I2C register 0x0F, before download firmware, need to write 0x07 then 0x05 to it,
   after download firmware, need to write 0x04 to it to let DSP running*/
int im501_write_and_check(uint8_t reg, uint8_t val)
{
    uint8_t ret_val;

    if (SUCCESS != im501_i2c_reg_write(reg, val, 1)) {
        ESP_LOGE(IM501_TAG, "in %s line %d", __func__, __LINE__);
        return E_ERROR;
    }

    if (SUCCESS != im501_i2c_reg_read(reg, (uint8_t*)&ret_val, 1)) {
        ESP_LOGE(IM501_TAG, "in %s line %d", __func__, __LINE__);
        return E_ERROR;
    }

    ESP_LOGI(IM501_TAG, "write %d, read back %d\n", val, ret_val);

    if (ret_val != val) {
        ESP_LOGE(IM501_TAG, "in %s line %d", __func__, __LINE__);
        return E_ERROR;
    }

    return SUCCESS;
}

/*read frame counter to check DSP is working or not*/
int check_dsp_status(void)
{
    uint8_t framecount1[4], framecount2[4];
    /*
        uint32_t framecount1, framecount2;
    */
    uint16_t framecount3, framecount4;
    uint16_t err = SUCCESS;

    err = im501_i2c_mem_read(TO_DSP_FRAMECOUNTER_ADDR, framecount1);

    if (err != SUCCESS) {
        ESP_LOGE(IM501_TAG, "in %s line %d", __func__, __LINE__);
        return err;
    }

    im501_clock_delay(500);

    err = im501_i2c_mem_read(TO_DSP_FRAMECOUNTER_ADDR, framecount2);

    if (err != SUCCESS) {
        ESP_LOGE(IM501_TAG, "in %s line %d", __func__, __LINE__);
        return err;
    }

    ESP_LOGI(IM501_TAG, "%x %x %x %x\n", framecount1[0], framecount1[1], framecount1[2], framecount1[3]);
    ESP_LOGI(IM501_TAG, "%x %x %x %x\n", framecount2[0], framecount2[1], framecount2[2], framecount2[3]);
    framecount3 = ((uint8_t)framecount1[2]) + (((uint8_t)framecount1[3]) << 8);
    framecount4 = ((uint8_t)framecount2[2]) + (((uint8_t)framecount2[3]) << 8);
    ESP_LOGI(IM501_TAG, "framecount3=%x, framecount4=%x\n", framecount3, framecount4);

    if (framecount3 != framecount4) {
        return SUCCESS;
    } else {
        ESP_LOGE(IM501_TAG, "in %s line %d", __func__, __LINE__);
        return E_ERROR;
    }

    return SUCCESS;
}

int im501_download_mem_data_fast(const uint8_t* pdata, uint32_t mem_addr, uint32_t mem_size, uint8_t unit)
{
    uint16_t i;
    uint16_t ret = SUCCESS;
    uint16_t a, b;
    uint32_t offset = 0;

    if ((pdata == NULL) || (mem_size == 0)) {
        return E_ERROR;
    }

    if (unit != 4 && unit != 2) {
        return E_ERROR;
    }

    a = mem_size / CSR_BLOCKSIZE;
    b = mem_size % CSR_BLOCKSIZE;

    for (i = 0; i < a; i++) {
        offset = CSR_BLOCKSIZE * i;
        ret = im501_i2c_mem_burst_write(mem_addr + offset, (uint8_t*)(pdata + offset), CSR_BLOCKSIZE);

        if (ret != SUCCESS) {
            return E_ERROR;
        }
    }

    if (b != 0) {
        offset = CSR_BLOCKSIZE * a;

        if (b == unit) {
            return im501_i2c_mem_write(mem_addr + offset, (uint8_t*)(pdata + offset));
        } else if (b > unit) {
            return im501_i2c_mem_burst_write(mem_addr + offset, (uint8_t*)(pdata + offset), b);
        } else {
            return E_ERROR;
        }
    }

    return SUCCESS;
}

int im501_download_mem_data_slow(const uint8_t* pdata, uint32_t mem_addr, uint32_t mem_size, uint8_t unit)
{
    uint32_t i, j;
    uint16_t ret = SUCCESS;

    if (unit != 4 && unit != 2) {
        return E_ERROR;
    }

    j = mem_size / unit;

    if (j != 0) {
        for (i = 0; i < j; i++) {
            ret = im501_i2c_mem_write(mem_addr + i * unit, pdata + i * unit);
        }
    } else {
        ret = E_ERROR; /*IRAM data size must be larger or equal to 4, DRAM data size must be larger or equal to 2*/
    }

    return ret;
}

int im501_download_mem_data(const uint8_t* pdata_ptr, uint32_t mem_addr, uint16_t mem_type, uint32_t mem_size, uint16_t f_load)
{
    uint16_t            ret = SUCCESS;

    if (mem_size == 0) {
        return SUCCESS;
    }

    if (pdata_ptr == NULL) {
        return E_ERROR;
    }

    if (mem_type == MEM_TYPE_IRAM) {
        if (f_load == TRUE) {
            return im501_download_mem_data_fast(pdata_ptr, mem_addr, mem_size, 4);
        } else {
            return im501_download_mem_data_slow(pdata_ptr, mem_addr, mem_size, 4);
        }
    } else if (mem_type == MEM_TYPE_DRAM) {
        if (f_load == TRUE) {
            return im501_download_mem_data_fast(pdata_ptr, mem_addr, mem_size, 2);
        } else {
            return im501_download_mem_data_slow(pdata_ptr, mem_addr, mem_size, 2);
        }
    } else {
        ret = E_ERROR;
    }

    return ret;
}

//#include "driver/i2s.h"
int im501_pdm_clki_set(uint16_t set_flag, uint32_t pdm_clki_rate)
{
    if (set_flag == NORMAL_MODE) {
        ESP_LOGI(IM501_TAG, "set the pdm_clki %d \n", pdm_clki_rate);
//        i2s_set_clk((i2s_port_t)0, pdm_clki_rate, 16, 2);
        im501_clock_delay(10);
    } else {
        ESP_LOGI(IM501_TAG, "unset the pdm_clki\n");
        im501_clock_delay(10);
    }

    return SUCCESS;
}

int im501_parse_to_host_command(to_host_cmd cmd)
{
    uint16_t err = SUCCESS;
    uint8_t* pdata;
    uint32_t pdm_clki_rate = (1L << 21);
    uint8_t  cmd_status;
    uint8_t cmd_cmd_type;

    cmd_status = cmd.attri_cmdbyte_status >> 31;
    cmd_cmd_type = (cmd.attri_cmdbyte_status >> 24) | 0x7F;

    /*ESP_LOGI(IM501_TAG, "cmd_byte = %#x\n", cmd.cmd_byte);*/
    if ((cmd_status == 1) && (cmd_cmd_type == TO_HOST_CMD_DATA_BUF_RDY)) {
        /*Reuest host to read To-Host Buffer-Fast*/
        /*do not process voice transfer on i2c*/
    } else { /*treat all other interrupt as keyword detect*/
        /*if(cmd.cmd_byte == TO_HOST_CMD_KEYWORD_DET) {//Info host Keywords detected*/
        /*ESP_LOGI(IM501_TAG, "im501_vbuf_trans_status = %d\n", im501_vbuf_trans_status);*/
        if ((im501_dsp_mode_old == POWER_SAVING_MODE)) { /*only available when not transfer voice and in PSM*/
            im501_host_irqstatus = 1;

            /*to change the iM501 from PSM to Normal mode, and to keep the same setting
              as the mode before changing to PSM, the Host just needs to turn on the PDMCLKI,
              and no additional command is needed.
            */
            im501_pdm_clki_set(NORMAL_MODE, pdm_clki_rate);
            im501_dsp_mode_old = NORMAL_MODE;
            im501_clock_delay(5);

            ESP_LOGI(IM501_TAG, "## keyword detected");

            /*cmd.status = 0;*/
            /*Should be modified in accordance with customers scenario definitions.*/
            pdata = (uint8_t*) & (cmd.attri_cmdbyte_status);
            pdata[3] &= 0x7F;   /*changes the bit-31 to 0 to indicate that the hot has finished with the task.*/
            /*ESP_LOGI(IM501_TAG, "pdata[%#x, %#x, %#x, %#x]\n", pdata[0], pdata[1], pdata[2], pdata[3]);*/
            err = im501_download_mem_data(pdata, TO_HOST_CMD_ADDR, MEM_TYPE_DRAM, 4, FALSE);

            if (err != SUCCESS) {
                ESP_LOGE(IM501_TAG, "in %s, (%d)", __func__, __LINE__);
                return err;
            }
        } else {
            ESP_LOGI(IM501_TAG, "Do not process other interrupt during process keyword detection and oneshot, or DSP is in normal mode.\n");
        }
    }

    return err;
}

int im501_send_to_dsp_command(to_501_cmd cmd)
{
    uint16_t err;
    uint16_t i;
    uint8_t pTempData[8];
    uint8_t* pdata;

    /*ESP_LOGI(IM501_TAG, "attri=%#x, cmd_byte_ext=%#x, status=%#x, cmd_byte=%#x\n", cmd.attri, cmd.cmd_byte_ext, cmd.status, cmd.cmd_byte);*/
    pdata = (uint8_t*) &cmd;
    /*ESP_LOGI(IM501_TAG, "pdata[%#x, %#x, %#x, %#x, %#x]\n", pdata[0], pdata[1], pdata[2], pdata[3], pdata[4]);*/
    /*ESP_LOGI(IM501_TAG, "attr=%#x, ext=%#x, status=%#x, cmd=%#x\n", cmd.attri, cmd.cmd_byte_ext, cmd.status, cmd.cmd_byte);*/

    err = im501_download_mem_data(pdata, TO_DSP_CMD_ADDR, MEM_TYPE_DRAM, 4, FALSE);

    if (err != SUCCESS) {
        ESP_LOGE(IM501_TAG, "call im501_spi_write_dram() return error(%d)\n", err);
        return err;
    }

    err = im501_i2c_reg_write(IM501_I2C_IRQ_REG, cmd.cmd_byte, 1); /*generate interrupt to DSP*/

    if (err != SUCCESS) {
        ESP_LOGE(IM501_TAG, "call im501_spi_write_reg() return error(%d)\n", err);
        return err;
    }

    memset(pTempData, 0, 8);

    for (i = 0; i < 50; i++) {    /*wait for (50*100us = 5ms) to check if DSP finished */
        err = im501_i2c_mem_read(TO_DSP_CMD_ADDR, pTempData);

        if (err != SUCCESS) {
            ESP_LOGE(IM501_TAG, "call im501_spi_read_dram() return error(%d)\n", err);
            return err;
        }

        ESP_LOGI(IM501_TAG, "pTempData[%#x, %#x, %#x, %#x]\n", pTempData[0], pTempData[1], pTempData[2], pTempData[3]);

        if ((pTempData[3] >> 7) != 0) {
            err = E_ERROR;
        } else {
            err = SUCCESS;
            break;
        }

        im501_clock_delay(5);
    }

    return err;
}

int im501_send_message_to_dsp(uint8_t cmd_index, uint32_t para)
{
    uint16_t err = SUCCESS;
    uint16_t i = 0;
    to_501_cmd    cmd;

    cmd.attri_cmdbyteext_status = para;
    cmd.cmd_byte = ((cmd_index & 0x3F) << 2) | 0x01; /*D[1] : "1", interrupt DSP. This bit generates NMI (non-mask-able interrupt), D[0]: "1" generate mask-able interrupt*/

    ESP_LOGI(IM501_TAG, "attri=%x, cmd_byte_ext=%x, status=%x, cmd_byte=%x\n",
             (cmd.attri_cmdbyteext_status & 0xFFFFFF), (uint8_t)((cmd.attri_cmdbyteext_status >> 24) & 0x7F),
             (uint8_t)(cmd.attri_cmdbyteext_status >> 31), cmd.cmd_byte);

    for (i = 0 ; i < 50 ; i++) {
        err = im501_send_to_dsp_command(cmd);

        if (err == SUCCESS) {
            break;
        } else {
            ESP_LOGE(IM501_TAG, "fail to send message(%02x) to dsp. err=%d\n", cmd_index >> 2, err);
        }
    }

    return err;
}

int request_enter_psm(void)
{
    uint16_t err = SUCCESS;

    if (im501_dsp_mode_old != POWER_SAVING_MODE) { /*The current dsp mode is not PSM.*/
        ESP_LOGI(IM501_TAG, "im501_dsp_mode_old = %d. iM501 is going to sleep.\n", im501_dsp_mode_old);
        ESP_LOGI(IM501_TAG, "the command is %#x\n", TO_DSP_CMD_REQ_ENTER_PSM);
        err = im501_send_message_to_dsp(TO_DSP_CMD_REQ_ENTER_PSM, 0);

        if (SUCCESS != err) {
            ESP_LOGE(IM501_TAG, "error occurs when switch to power saving mode, error = %d\n", err);
        }

        im501_clock_delay(2500);
        im501_pdm_clki_set(POWER_SAVING_MODE, 0);
        im501_clock_delay(20);

        im501_dsp_mode_old = POWER_SAVING_MODE;
        ESP_LOGI(IM501_TAG, "im501_dsp_mode_old = %d\n", im501_dsp_mode_old);
    }

    return err;
}

int request_enter_normal(void)
{
    uint16_t err = SUCCESS;

    if (im501_dsp_mode_old != NORMAL_MODE) { /*The current dsp mode is not NORMAL*/
        ESP_LOGI(IM501_TAG, "%s: entering...\n", __func__);

        im501_clock_delay(20);
        im501_pdm_clki_set(NORMAL_MODE, 0);
        im501_clock_delay(2500);

        err = im501_send_message_to_dsp(TO_DSP_CMD_REQ_ENTER_NORMAL, 0);

        if (SUCCESS != err) {
            ESP_LOGE(IM501_TAG, "%s: error occurs when switch to normal mode, error = %d\n", __func__, err);
        }

        im501_dsp_mode_old = NORMAL_MODE;
        ESP_LOGI(IM501_TAG, "%s: im501_dsp_mode_old = %d\n", __func__, im501_dsp_mode_old);
    }

    return err;
}

int request_enter_hw_bypass(void)
{
    uint16_t err = SUCCESS;

    ESP_LOGI(IM501_TAG, "%s: entering...\n", __func__);

    if (im501_dsp_mode_old != HWBYPASS_MODE) { /*The current dsp mode is not HWBYPASS_MODE*/
        err = im501_send_message_to_dsp(TO_DSP_CMD_REQ_ENTER_HW_BYPASS, 0);

        if (SUCCESS != err) {
            ESP_LOGE(IM501_TAG, "%s: error occurs when switch to normal mode, error = %d\n", __func__, err);
        }

        im501_clock_delay(20);
        im501_pdm_clki_set(NORMAL_MODE, 0);
        im501_clock_delay(2500);

        im501_dsp_mode_old = HWBYPASS_MODE;
        ESP_LOGI(IM501_TAG, "%s: im501_dsp_mode_old = %d\n", __func__, im501_dsp_mode_old);
    }

    return err;
}

static void im501_irq_handling_work(void)
{
    uint16_t err;
    to_host_cmd   cmd;
    uint8_t* pdata;

    /*ESP_LOGI(IM501_TAG, "%s: entering...\n", __func__);*/
    pdata = (uint8_t*) & (cmd.attri_cmdbyte_status);
    err = im501_i2c_mem_read(TO_HOST_CMD_ADDR, pdata);

    if (err != SUCCESS) {
        ESP_LOGE(IM501_TAG, "%s: error occurs when get data from DRAM, error=%d\n", __func__, err);
        return ;
    }

    if ((im501_host_irqstatus == 1)) {
        ESP_LOGI(IM501_TAG, "%s: pdata[%#x, %#x, %#x, %#x]\n", __func__, pdata[0], pdata[1], pdata[2], pdata[3]);
    }

    if ((pdata[3] >> 7) != 1) {
        ESP_LOGI(IM501_TAG, "%s: got wrong command from DSP.\n", __func__);
    }

    err = im501_parse_to_host_command(cmd);

    if (err != SUCCESS) {
        ESP_LOGE(IM501_TAG, "%s: error occurs when calling parse_to_host_command(), error=%d\n", __func__, err);
    }

    return ;
}
#endif
