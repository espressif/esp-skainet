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
#include "driver/spi_master.h"
#include "soc/gpio_struct.h"
#include "driver/gpio.h"
#include "esp_partition.h"
#include "EspAudioAlloc.h"
#include "im501_spi.h"
#include "im501_SPI_driver.h"

extern SemaphoreHandle_t dsp_asr_sema;

#define IM501_TAG "IM501_DRIVER_SPI"

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

#define mdelay(x) vTaskDelay(x/portTICK_RATE_MS)
#define msleep(x) vTaskDelay(x/portTICK_RATE_MS)

#define ENOMEM                           2

//#define IM501_SPI_BUF_LEN              48 //changed to 48 due to dma issue
//#define IM501_SPI_BUF_LEN              64 //changed to 64 due to dma issue
#define IM501_SPI_BUF_LEN                4000   //changed to 4000 due to dma issue

#define INTERNAL_SIZE                   (4000)

#define  IM501_SPI_CMD_DM_WR             0x05
#define  IM501_SPI_CMD_DM_RD             0x01
#define  IM501_SPI_CMD_IM_WR             0x04
#define  IM501_SPI_CMD_IM_RD             0x00
#define  IM501_SPI_CMD_REG_WR            0x06
#define  IM501_SPI_CMD_REG_RD            0x02

#define FW_RD_CHECK
#define FW_BURST_RD_CHECK

#define DEVICE_NAME  "fm_smp"
#define MAX_KFIFO_BUFFER_SIZE       (131072*2) /* >4 seconds */
#define CODEC2IM501_PDM_CLKI
#define DSP_IRQ_DETECT_PIN_SEL      ((1ULL)<<DSP_IRQ_DETECT_PIN)

voice_buf  voice_buf_data;
int im501_dsp_mode = -1;
int ap_sleep_flag = 0;
static spi_device_handle_t im501_spi;
static int im501_host_irqstatus = 0;    // To notify the HAL about the incoming irq.

typedef struct to_im501_cmd_t {
    unsigned int  attri        : 24 ;
    unsigned int  cmd_byte_ext : 7 ;
    unsigned int  status       : 1 ;
    int cmd_byte;
} to_501_cmd;

typedef struct to_host_cmd_t {
    uint32_t   attri    : 24;
    uint32_t   cmd_byte : 7;
    uint32_t   status   : 1;
} to_host_cmd;


static void esp32_spi_init(void *handle)
{
    int ret;
    spi_bus_config_t buscfg = {
        .miso_io_num = DSP_FT_SPI_MISO,
        .mosi_io_num = DSP_FT_SPI_MOSI,
        .sclk_io_num = DSP_FT_SPI_CLK,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1
    };
    spi_device_interface_config_t devcfg = {
        .clock_speed_hz = 10 * 1000 * 1000,         //Clock out at 10 MHz
        .mode = 0,                              //SPI mode 0
        .spics_io_num = DSP_FT_SPI_CS,             //CS pin
        .queue_size = 7,                        //We want to be able to queue 7 transactions at a time
        .pre_cb = NULL, //Specify pre-transfer callback to handle D/C line
    };
    //Initialize the SPI bus
    ret = spi_bus_initialize(HSPI_HOST, &buscfg, 1);
    assert(ret == ESP_OK);
    //Attach the LCD to the SPI bus
    ret = spi_bus_add_device(HSPI_HOST, &devcfg, (struct spi_device_t **)&handle);
    assert(ret == ESP_OK);
}

static void esp32_spi_deinit(void *handle)
{
    spi_bus_free(HSPI_HOST);
    spi_bus_remove_device(handle);
    handle = NULL;
}

esp_partition_t *im501_partition_init(int subType)
{
    esp_partition_t *DspBinPartition = NULL;
    DspBinPartition = (esp_partition_t *)esp_partition_find_first(ESP_PARTITION_TYPE_DATA, subType, NULL);

    if (NULL == DspBinPartition) {
        ESP_LOGE(IM501_TAG, "Can not find dsp-partition");
        return NULL;
    }

    ESP_LOGD("DspPartition", "type[%x]", DspBinPartition->type);
    ESP_LOGD("DspPartition", "subtype[%x]", DspBinPartition->subtype);
    ESP_LOGI("DspPartition", "address:0x%x", DspBinPartition->address);
    ESP_LOGD("DspPartition", "size:0x%x", DspBinPartition->size);
    ESP_LOGD("DspPartition", "labe:%s", DspBinPartition->label);

    return DspBinPartition;
}

int im501_spi_read_reg(uint8_t reg, uint8_t *val)
{
    spi_transaction_t t;
    int ret = 0;
    memset(&t, 0, sizeof(t));

    t.length = 3 * 8;                     //Command is 8 bits
    t.rxlength = 1 * 8;                     //Command is 8 bits
    t.flags = SPI_TRANS_USE_TXDATA | SPI_TRANS_USE_RXDATA;
    t.tx_data[0] = IM501_SPI_CMD_REG_RD; //0x00 read  iram0 cmd
    t.tx_data[1] = reg;
    t.tx_data[2] = 0;

    ret = spi_device_transmit(im501_spi, &t);
    *val = t.rx_data[2];

    return ret;
}


int im501_spi_write_reg(uint8_t reg, uint8_t val)
{
    spi_transaction_t t;
    int ret = 0;
    memset(&t, 0, sizeof(t));

    t.length = 3 * 8;                     //Command is 8 bits
    t.flags = SPI_TRANS_USE_TXDATA;
    t.tx_data[0] = IM501_SPI_CMD_REG_WR; //0x00 read  iram0 cmd
    t.tx_data[1] = reg;
    t.tx_data[2] = val;

    ret = spi_device_transmit(im501_spi, &t);

    if (ret) {
        ESP_LOGE(IM501_TAG, "in %s line %d", __func__, __LINE__);
    }

    return ret;
}

int im501_spi_read_dram(uint32_t addr, void *pdata)
{
    spi_transaction_t t[3] = {0};
    spi_transaction_t *r;
    int ret = 0;

    uint8_t spi_cmd, addr_msb;

    addr_msb = (addr >> 24) & 0xff;

    if (addr_msb == 0x10) {
        spi_cmd = IM501_SPI_CMD_IM_RD;
    } else if (addr_msb == 0xF) {
        spi_cmd = IM501_SPI_CMD_DM_RD;
    } else {
        ESP_LOGE(IM501_TAG, "spi_cmd not initialized in %s line %d", __func__, __LINE__);
        return -1;
    }

    t[0].length = 4 * 8;                     //Command is 8 bits
    t[0].flags = SPI_TRANS_USE_TXDATA;
    t[0].tx_data[0] = spi_cmd; //0x00 read  iram0 cmd
    t[0].tx_data[1] = addr & 0xFF;
    t[0].tx_data[2] = (addr >> 8) & 0xFF;
    t[0].tx_data[3] = (addr >> 16) & 0xFF;

    t[1].length = 2 * 8;                     //Command is 8 bits
    t[1].flags = SPI_TRANS_USE_TXDATA;
    t[1].tx_data[0] = 2;
    t[1].tx_data[1] = 0;

    t[2].length = 4 * 8;                     //Command is 8 bits
    t[2].flags = 0;
    t[2].rx_buffer = pdata;

    for (int i = 0; i < 3; i++) {
        ret |= spi_device_queue_trans(im501_spi, &t[i], portMAX_DELAY);
    }

    for (int i = 0; i < 3; i++) {
        r = &t[i];
        ret |= spi_device_get_trans_result(im501_spi, &r, portMAX_DELAY);
    }

    if (ret) {
        ESP_LOGE(IM501_TAG, "in %s line %d", __func__, __LINE__);
    }

    return ret;
}

int im501_spi_write_dram(uint32_t addr, uint8_t *pdata)
{
    spi_transaction_t t[3] = {0};
    spi_transaction_t *r;
    int ret = 0;

    t[0].length = 4 * 8;                     //Command is 8 bits
    t[0].flags = SPI_TRANS_USE_TXDATA;
    t[0].tx_data[0] = IM501_SPI_CMD_DM_WR; //0x00 read  iram0 cmd
    t[0].tx_data[1] = addr & 0xFF;
    t[0].tx_data[2] = (addr >> 8) & 0xFF;
    t[0].tx_data[3] = (addr >> 16) & 0xFF;

    t[1].length = 4 * 8;                     //Command is 8 bits
    t[1].flags = SPI_TRANS_USE_TXDATA;
    t[1].tx_data[0] = 2;
    t[1].tx_data[1] = 0;
    t[1].tx_data[2] = *(pdata);
    t[1].tx_data[3] = *(pdata + 1);

    t[2].length = 2 * 8;                     //Command is 8 bits
    t[2].flags = SPI_TRANS_USE_TXDATA;
    t[2].tx_data[0] = *(pdata + 2);
    t[2].tx_data[1] = *(pdata + 3);

    for (int i = 0; i < 3; i++) {
        ret |= spi_device_queue_trans(im501_spi, &t[i], portMAX_DELAY);
    }

    for (int i = 0; i < 3; i++) {
        ret |= spi_device_get_trans_result(im501_spi, &r, portMAX_DELAY);
    }

    return ret;
}

int im501_spi_write_dram_2byte(uint32_t addr, uint16_t pdata)
{
    spi_transaction_t t[2] = {0};
    spi_transaction_t *r;
    int ret = 0;

    t[0].length = 4 * 8;                     //Command is 8 bits
    t[0].flags = SPI_TRANS_USE_TXDATA;
    t[0].tx_data[0] = IM501_SPI_CMD_DM_WR;   //0x00 read  iram0 cmd
    t[0].tx_data[1] = addr & 0xFF;
    t[0].tx_data[2] = (addr >> 8) & 0xFF;
    t[0].tx_data[3] = (addr >> 16) & 0xFF;

    t[1].length = 4 * 8;                     //Command is 8 bits
    t[1].flags = SPI_TRANS_USE_TXDATA;
    t[1].tx_data[0] = 1;
    t[1].tx_data[1] = 0;
    t[1].tx_data[2] = (uint8_t)pdata;
    t[1].tx_data[3] = (uint8_t)(pdata >> 8);

    for (int i = 0; i < 2; i++) {
        ret |= spi_device_queue_trans(im501_spi, &t[i], portMAX_DELAY);
    }

    for (int i = 0; i < 2; i++) {
        ret |= spi_device_get_trans_result(im501_spi, &r, portMAX_DELAY);
    }

    return ret;
}

/**
 * im501_spi_burst_read_dram - Read data from SPI by im501 dsp memory address.
 * @addr: Start address.
 * @rxbuf: Data Buffer for reading.
 * @len: Data length, it must be a multiple of 8.
 *
 * Returns true for success.
 */
int im501_spi_burst_read_dram(uint32_t addr, uint8_t *rxbuf, size_t len)
{
    spi_transaction_t t[2];
    spi_transaction_t *r;
    int ret = 0, counter = 0;

    uint8_t spi_cmd, addr_msb;
    uint8_t write_buf[6];
    unsigned int end = 0, offset = 0;
    memset(t, 0, sizeof(spi_transaction_t) * 2);

    addr_msb = (addr >> 24) & 0xff;

    if (addr_msb == 0x10) {
        spi_cmd = IM501_SPI_CMD_IM_RD;
        //ESP_LOGI(IM501_TAG, "%s-1: addr = %#x, spi_cmd = %d\n", __func__, addr, spi_cmd);
    } else if (addr_msb == 0xF) {
        spi_cmd = IM501_SPI_CMD_DM_RD;
        //ESP_LOGI(IM501_TAG, "%s-2: addr = %#x, spi_cmd = %d\n", __func__, addr, spi_cmd);
    } else {
        ESP_LOGE(IM501_TAG, "spi_cmd not initialized in %s line %d", __func__, __LINE__);
        return -1;
    }

    while (offset < len) {
        if (offset + IM501_SPI_BUF_LEN <= len) {
            end = IM501_SPI_BUF_LEN;
        } else {
            end = len % IM501_SPI_BUF_LEN;
        }

        write_buf[0] = spi_cmd;
        write_buf[1] = ((addr + offset) & 0x000000ff) >> 0;
        write_buf[2] = ((addr + offset) & 0x0000ff00) >> 8;
        write_buf[3] = ((addr + offset) & 0x00ff0000) >> 16;
        write_buf[4] = (end >> 1) & 0xff;
        write_buf[5] = (end >> (1 + 8)) & 0xff;

        t[0].length = 6 * 8;                     //Command is 8 bits
        t[0].flags = 0;
        t[0].tx_buffer = write_buf;
        t[1].length = end * 8;                     //Command is 8 bits
        t[1].flags = 0;
        t[1].rx_buffer = rxbuf + offset;


        ret = spi_device_queue_trans(im501_spi, &t[0], portMAX_DELAY);
        counter++;
        ret |= spi_device_queue_trans(im501_spi, &t[1], portMAX_DELAY);
        counter++;

        if (ret) {
            ESP_LOGE(IM501_TAG, "in %s line %d", __func__, __LINE__);
            return false;
        }

        offset += end;
    }

    for (int i = 0; i < counter; i++) {
        r = &t;
        ret |= spi_device_get_trans_result(im501_spi, &r, portMAX_DELAY);
    }

    if (ret) {
        ESP_LOGE(IM501_TAG, "in %s line %d", __func__, __LINE__);
        return false;
    }

    return 0;
}

/**
 * im501_spi_burst_write - Write data to SPI by im501 dsp memory address.
 * @addr: Start address.
 * @txbuf: Data Buffer for writng.
 * @len: Data length, it must be a multiple of 8.
 * @type: Firmware type, MSB and LSB, the iM501 firmware is in MSB, but the EFT firmware is in LSB.
 *
 * Returns true for success.
 */
int im501_spi_burst_write(uint32_t addr, const uint8_t *txbuf, size_t len, int fw_type)
{
    spi_transaction_t t;
    spi_transaction_t *r;
    int ret = 0;

    uint8_t spi_cmd, *write_buf;
    uint32_t offset = 0;
    int local_len, end, i, counter = 0;

    memset(&t, 0, sizeof(t));
    uint32_t addr_buf = 0x00000000;
    addr_buf = addr >> 24;

    spi_cmd = (addr_buf == 0x10) ? IM501_SPI_CMD_IM_WR : IM501_SPI_CMD_DM_WR;
    local_len = (len % 8) ? (((len / 8) + 1) * 8) : len;

    write_buf = EspAudioAllocInner(1, IM501_SPI_BUF_LEN + 6);

    if (write_buf == NULL) {
        return -ENOMEM;
    }

    write_buf[0] = spi_cmd;                                         //Assign the command byte first, since it is fixed.

    while (offset < local_len) {
        if (offset + IM501_SPI_BUF_LEN <= local_len) {
            end = IM501_SPI_BUF_LEN;
        } else {
            end = local_len % IM501_SPI_BUF_LEN;
        }

        write_buf[1] = ((addr + offset) & 0x000000ff) >> 0;     //The memory address
        write_buf[2] = ((addr + offset) & 0x0000ff00) >> 8;
        write_buf[3] = ((addr + offset) & 0x00ff0000) >> 16;
        write_buf[4] = ((end / 2) & 0x00ff) >> 0;               //The word counter low byte.
        write_buf[5] = ((end / 2) & 0xff00) >> 8;               //Assign the high order word counter, since it is fixed.

        if (fw_type == IM501_DSP_FW) {
            for (i = 0; i < end; i += 8) {
                write_buf[i + 13] = txbuf[offset + i + 0];
                write_buf[i + 12] = txbuf[offset + i + 1];
                write_buf[i + 11] = txbuf[offset + i + 2];
                write_buf[i + 10] = txbuf[offset + i + 3];
                write_buf[i + 9] = txbuf[offset + i + 4];
                write_buf[i + 8] = txbuf[offset + i + 5];
                write_buf[i + 7] = txbuf[offset + i + 6];
                write_buf[i + 6] = txbuf[offset + i + 7];
            }
        } else if (fw_type == IM501_EFT_FW) {
            memcpy(write_buf + 6, txbuf + offset, end);
//              for (i = 0; i < end; i += 8) {
//                  write_buf[i + 6] = txbuf[offset + i + 0];
//                  write_buf[i + 7] = txbuf[offset + i + 1];
//                  write_buf[i + 8] = txbuf[offset + i + 2];
//                  write_buf[i + 9] = txbuf[offset + i + 3];
//                  write_buf[i + 10] = txbuf[offset + i + 4];
//                  write_buf[i + 11] = txbuf[offset + i + 5];
//                  write_buf[i + 12] = txbuf[offset + i + 6];
//                  write_buf[i + 13] = txbuf[offset + i + 7];
//              }
        }

        t.length = (end + 6) * 8;                     //Command is 8 bits
        t.flags = 0;
        t.tx_buffer = write_buf;

        ret = spi_device_queue_trans(im501_spi, &t, portMAX_DELAY);
        counter++;

        if (ret) {
            ESP_LOGE(IM501_TAG, "in %s line %d", __func__, __LINE__);
            ret = -1;
            break;
        }

        offset += IM501_SPI_BUF_LEN;
    }

    for (i = 0; i < counter; i++) {
        ret |= spi_device_get_trans_result(im501_spi, &r, portMAX_DELAY);
    }

    if (ret) {
        ESP_LOGE(IM501_TAG, "in %s line %d", __func__, __LINE__);
        ret = -1;
    }

    free(write_buf);

    return ret;
}

int im501_send_to_dsp_command(to_501_cmd cmd)
{
    int err;
    unsigned int  i;
    uint8_t *pdata;

    //ESP_LOGI(IM501_TAG, "%s: attri=%#x, cmd_byte_ext=%#x, status=%#x, cmd_byte=%#x\n", __func__, cmd.attri, cmd.cmd_byte_ext, cmd.status, cmd.cmd_byte);
    pdata = (uint8_t *) &cmd;
    //ESP_LOGI(IM501_TAG, "%s: pdata[%#x, %#x, %#x, %#x, %#x]\n", __func__, pdata[0], pdata[1], pdata[2], pdata[3], pdata[4]);
    //ESP_LOGI(IM501_TAG, "%s: attr=%#x, ext=%#x, status=%#x, cmd=%#x\n", __func__, cmd.attri, cmd.cmd_byte_ext, cmd.status, cmd.cmd_byte);

    err = im501_spi_write_dram(TO_DSP_CMD_ADDR, pdata);

    if (err != 0) {
        return err;
    }

    err = im501_spi_write_reg(0x01,  cmd.cmd_byte);   //generate interrupt to DSP

    if (err != 0) {
        return err;
    }

    //Mar.17,2016, allocate 4-byte space for dsp read.
    pdata = EspAudioAlloc(1, 4);

    if (pdata == NULL) {
        return -ENOMEM;
    }

    for (i = 0; i < 50; i++) {   //wait for (50*100us = 5ms) to check if DSP finished
        err = im501_spi_read_dram(TO_DSP_CMD_ADDR, pdata);

        if (err != 0) {
            return err;
        }

        //ESP_LOGI(IM501_TAG, "%s: pdata[%#x, %#x, %#x, %#x]\n", __func__, pdata[0], pdata[1], pdata[2], pdata[3]);
        cmd.status = pdata[3] >> 7;
        cmd.cmd_byte_ext = pdata[3] & 0x7F;
        cmd.attri = pdata[0] | (pdata[1] * 256) | (pdata[2] * 256 * 256);
        ESP_LOGI(IM501_TAG, "%s: cmd[%#x, %#x, %#x]\n", __func__, cmd.status, cmd.cmd_byte_ext, cmd.attri);

        if (cmd.status != 0) {
            err = TO_501_CMD_ERR;
        } else {
            err = 0;
            break;
        }

        msleep(1);
    }

    free(pdata);

    return err;
}

int write_cmd_to_im501(int cmd_index, unsigned int para)
{

    int err;
    to_501_cmd    cmd;

    cmd.attri    = para & 0xFFFFFF ;
    cmd.cmd_byte_ext = (para >> 24) & 0x7F;
    cmd.status   = 1;

    cmd.cmd_byte = cmd_index;//((cmd_index & 0x3F) << 2) | 0x01; //D[1] : "1", interrupt DSP. This bit generates NMI (non-mask-able interrupt), D[0]: "1" generate mask-able interrupt

    //ESP_LOGI(IM501_TAG, "%s: attri=%#x, cmd_byte_ext=%#x, status=%#x, cmd_byte=%#x\n", __func__, cmd.attri, cmd.cmd_byte_ext, cmd.status, cmd.cmd_byte);
    err = im501_send_to_dsp_command(cmd);

    return err;

}

/**
 * This function should be implemented by customers as a callback function.
 * The following is just a sample to control the PDM_CLKI on/off through UIF.
 * The parameter @pdm_clki_rate is reserved for future use.
 */
int codec2im501_pdm_clki_set(bool set_flag, uint32_t pdm_clki_rate)
{
//  if(set_flag) {
//      ESP_LOGI(IM501_TAG, "%s: set the pdm_clki with gpio_number = %d\n", __func__, im501_data->pdm_clki_gpio);
//      gpio_set_value(im501_data->pdm_clki_gpio, 1);
//      mdelay(10);
//  } else {
//      ESP_LOGI(IM501_TAG, "%s: unset the pdm_clki with gpio_number = %d\n", __func__, im501_data->pdm_clki_gpio);
//      gpio_set_value(im501_data->pdm_clki_gpio, 0);
//      mdelay(10);
//  }
    return 0;
}

int request_start_voice_buf_trans(void)
{
    int err = 0;

//  ESP_LOGI(IM501_TAG, "%s: im501_vbuf_trans_status = %d\n", __func__, im501_vbuf_trans_status);
    err = write_cmd_to_im501(TO_DSP_CMD_REQ_START_BUF_TRANS, 0);
    return err;
}

int request_stop_voice_buf_trans(void)
{
    int err = 0;

    err = write_cmd_to_im501(TO_DSP_CMD_REQ_STOP_BUF_TRANS, 0);

    return err;
}

int parse_to_host_command(to_host_cmd cmd)
{
    int err = 0;
    uint8_t *pdata;
    uint32_t pdm_clki_rate = 2 * 1000 * 1000;
    uint32_t address;

    ESP_LOGI(IM501_TAG, "IRQ line %d cmd_byte = %#x\n", __LINE__, cmd.cmd_byte);

    if (cmd.cmd_byte == TO_HOST_CMD_KEYWORD_DET) { //Info host Keywords detected

        im501_host_irqstatus = 1;
#ifdef CODEC2IM501_PDM_CLKI
        //to change the iM501 from PSM to Normal mode, and to keep the same setting
        //as the mode before changing to PSM, the Host just needs to turn on the PDMCLKI,
        //and no additional command is needed.
        codec2im501_pdm_clki_set(1, pdm_clki_rate);
        im501_dsp_mode = 1;
        mdelay(5);
#endif
        ESP_LOGI(IM501_TAG, "%s: ap_sleep_flag = %d, im501_dsp_mode = %d\n", __func__, ap_sleep_flag, im501_dsp_mode);

        if (ap_sleep_flag) {
            esp32_spi_init(im501_spi);
            msleep(10);
            ap_sleep_flag = 0;
        }

        //cmd.status = 0;
        //Should be modified in accordance with customers scenario definitions.
        pdata = (uint8_t *)&cmd;
        pdata[3] &= 0x7F;   //changes the bit-31 to 0 to indicate that the hot has finished with the task.
        ESP_LOGI(IM501_TAG, "%s: pdata[%#x, %#x, %#x, %#x]\n", __func__, pdata[0], pdata[1], pdata[2], pdata[3]);
        err = im501_spi_write_dram(TO_HOST_CMD_ADDR, pdata);

        if (err != 0) {
            ESP_LOGE(IM501_TAG, "in %s line %d", __func__, __LINE__);
            return err;
        }

        request_start_voice_buf_trans();
    } else if (cmd.cmd_byte == TO_HOST_CMD_DATA_BUF_RDY) { //Reuest host to read To-Host Buffer-Fast
        voice_buf_data.index   = (cmd.attri >> 8) & 0xFFFF; //package index

        if ((cmd.attri & 0xFF) == 0xF0) {
            address = HW_VOICE_BUF_START; //BANK0 address
        } else if ((cmd.attri & 0xFF) == 0xF9) {
            address = HW_VOICE_BUF_START + HW_VOICE_BUF_BANK_SIZE ; //BANK1 address
        }

        ESP_LOGI(IM501_TAG, "ADD queue here to recieve recording data");

//        sendqueue();
//        err = im501_spi_burst_read_dram( address, buf, HW_VOICE_BUF_BANK_SIZE );
//        err |= request_stop_voice_buf_trans();

        if (err != NO_ERR) {
            return err;
        }

        //cmd.status = 0;
        //Should be modified in accordance with customers scenario definitions.
        pdata = (uint8_t *)&cmd;
        pdata[3] &= 0x7F;   //changes the bit-31 to 0 to indicate that the hot has finished with the task.
        //ESP_LOGI(IM501_TAG, "%s: pdata[%#x, %#x, %#x, %#x]\n", __func__, pdata[0], pdata[1], pdata[2], pdata[3]);
        err = im501_spi_write_dram(TO_HOST_CMD_ADDR, pdata);

        if (err != 0) {
            return err;
        }

    } else {
        err = 2;
    }

    return err;
}

//power saving mode
int request_enter_psm(void)
{
    int err = 0;

//      request_stop_voice_buf_trans(); //Call this function to guarantee there is no voice transferring.

    if (im501_dsp_mode != 0) { //The current dsp mode is not PSM.
        ESP_LOGI(IM501_TAG, "%s-1: ap_sleep_flag = %d, im501_dsp_mode = %d. iM501 is going to sleep.\n", __func__, ap_sleep_flag, im501_dsp_mode);
        ESP_LOGI(IM501_TAG, "%s: the command is %#x\n", __func__, TO_DSP_CMD_REQ_ENTER_PSM);
        err = write_cmd_to_im501(TO_DSP_CMD_REQ_ENTER_PSM, 0xc0000000);

#ifdef CODEC2IM501_PDM_CLKI
        mdelay(5);
        codec2im501_pdm_clki_set(0, 0);
        mdelay(5);
#endif
        ap_sleep_flag = 1;
        im501_dsp_mode = 0;
        ESP_LOGI(IM501_TAG, "%s-2: ap_sleep_flag = %d, im501_dsp_mode = %d\n", __func__, ap_sleep_flag, im501_dsp_mode);
        esp32_spi_deinit(im501_spi);
    }

    return err;
}

void spi_request_enter_normal(void)
{

    if (im501_dsp_mode != 1) { //The current dsp mode is not NORMAL
        ESP_LOGI(IM501_TAG, "%s: entering...\n", __func__);
#ifdef CODEC2IM501_PDM_CLKI
        mdelay(5);
        codec2im501_pdm_clki_set(1, 0);
        mdelay(5);
#endif
        ap_sleep_flag = 0;
        im501_dsp_mode = 1;
        ESP_LOGI(IM501_TAG, "%s: ap_sleep_flag = %d, im501_dsp_mode = %d\n", __func__, ap_sleep_flag, im501_dsp_mode);
    }

}

static void im501_8byte_swap(uint8_t *rxbuf, uint32_t len)
{
    uint8_t local_buf[8];
    int i;

    for (i = 0; i < len; i += 8) {
        memcpy(local_buf, rxbuf + i, 8);

        rxbuf[i + 0] = local_buf[7];
        rxbuf[i + 1] = local_buf[6];
        rxbuf[i + 2] = local_buf[5];
        rxbuf[i + 3] = local_buf[4];
        rxbuf[i + 4] = local_buf[3];
        rxbuf[i + 5] = local_buf[2];
        rxbuf[i + 6] = local_buf[1];
        rxbuf[i + 7] = local_buf[0];
    }
}

static int im501_dsp_load_single_fw_file(int file_index, uint32_t addr, int fw_type)
{
    uint8_t *local_buf = NULL, *data = NULL;
    int i, ret = 0;

    esp_partition_t *DspBin = NULL;
    int index = 0, partitionOffset = 0, ret_len;
    index = 0x20 + file_index;

    // partition table
    DspBin = im501_partition_init(index);
    IM501_CHECK_NULL(DspBin, "partition init", -1);
    data = EspAudioAllocInner(1, INTERNAL_SIZE);
    IM501_CHECK_NULL(data, "EspAudioAllocInner", -1);

#ifdef FW_RD_CHECK
    local_buf = (uint8_t *)EspAudioAllocInner(1, INTERNAL_SIZE);

    if (!local_buf) {
        ESP_LOGE(IM501_TAG, "NO memory in %s, line: %d", __func__, __LINE__);
        ret = -1;
        goto END;
    }

#endif

    while (partitionOffset < DspBin->size) {
        if (partitionOffset + INTERNAL_SIZE < DspBin->size) {
            ret_len = INTERNAL_SIZE;
        } else {
            ret_len = DspBin->size - partitionOffset;
        }

        if (esp_partition_read(DspBin, partitionOffset, data, ret_len) != 0) {
            ESP_LOGE(IM501_TAG, "in %s, line: %d", __func__, __LINE__);
            break;
        }

        im501_spi_burst_write(addr + partitionOffset, data, ret_len, fw_type);

#ifdef FW_BURST_RD_CHECK
        im501_spi_burst_read_dram(addr + partitionOffset, local_buf, ret_len);

        if (fw_type == IM501_DSP_FW) {
            im501_8byte_swap(local_buf, ret_len);
        }

        for (i = 0; i < ret_len; i++) {
            if (local_buf[i] != data[i]) {
                ESP_LOGE(IM501_TAG, "%s: fw read %#x vs write %#x @ %#x\n", __func__, local_buf[i], data[i], addr + i);
                ret = -1;
                goto END;
            }
        }

#endif
        partitionOffset += ret_len;

    }

END:

    free(data);

    if (local_buf) {
        free(local_buf);
    }

    return ret;
}

static void im501_dsp_load_fw(void)
{
    ESP_LOGI(IM501_TAG, "%s: entering...\n", __func__);

    im501_dsp_load_single_fw_file(FILE_IRAM0_FM, 0x10000000, IM501_DSP_FW);
    im501_dsp_load_single_fw_file(FILE_DRAM0_FM, 0x0ffc0000, IM501_DSP_FW);
    im501_dsp_load_single_fw_file(FILE_DRAM1_FM, 0x0ffe0000, IM501_DSP_FW);
}

int im501_fw_loaded()
{
    uint32_t pdm_clki_rate = 2 * 1000 * 1000;
    int val = 0;

    ESP_LOGI(IM501_TAG, "%s: entering...\n", __func__);
#ifdef CODEC2IM501_PDM_CLKI
    //Commented out temporarily
    codec2im501_pdm_clki_set(1, pdm_clki_rate);
    mdelay(5); //Apply PDM_CLKI, and then wait for 1024 clock cycles
#endif

    im501_spi_write_reg(0x00, 0x07);
    im501_spi_read_reg(0x00, &val);

    if (val != 0x07) {
        ESP_LOGE(IM501_TAG, "%d is right.\n", val);
        return -1;
    }

    im501_spi_write_reg(0x00, 0x05);
    im501_spi_read_reg(0x00, &val);

    if (val != 0x05) {
        ESP_LOGE(IM501_TAG, "%d is right.line %d", val, __LINE__);
        return -1;
    }

    mdelay(1);
    im501_dsp_load_fw();
    mdelay(1);

    im501_spi_write_reg(0x00, 0x04);
    im501_spi_read_reg(0x00, &val);

    if (val != 4) {
        ESP_LOGE(IM501_TAG, "%d is right.\n", val);
        return -1;
    }

    im501_dsp_mode = 1;

    //Here after download the firmware, force DSP enter PSM mode for testing.
    //Should be changed according to the real requirement.
    //request_enter_psm();

    return 0;
}

static void im501_irq_handling_work()
{
    int err;
    to_host_cmd   cmd;
    uint8_t *pdata;
    uint32_t spi_speed = 1000000;

    if (!im501_dsp_mode) {
        ESP_LOGI(IM501_TAG, "%s: iM501 is in PSM, set the im501_spi speed to %d\n", __func__, spi_speed);
        esp32_spi_init(im501_spi);

        msleep(10);
    }

    pdata = (uint8_t *)&cmd;
    err = im501_spi_read_dram(TO_HOST_CMD_ADDR, pdata);

    if (err != 0) {
        ESP_LOGE(IM501_TAG, "in %s line %d", __func__, __LINE__);
        return ;
    }

    //ESP_LOGI(IM501_TAG, "%s: pdata[%#x, %#x, %#x, %#x]\n", __func__, pdata[0], pdata[1], pdata[2], pdata[3]);
    cmd.status = pdata[3] >> 7;
    cmd.cmd_byte = pdata[3] & 0x7F;
    cmd.attri = pdata[0] | (pdata[1] * 256) | (pdata[2] * 256 * 256);
    ESP_LOGI(IM501_TAG, "%s: cmd[%#x, %#x, %#x]\n", __func__, cmd.status, cmd.cmd_byte, cmd.attri);

    err = parse_to_host_command(cmd);

    if (err != 0) {
        ESP_LOGE(IM501_TAG, "in %s line %d", __func__, __LINE__);
        return ;
    }
}

static void IM501IntrHandler(void *arg)
{
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    int val = gpio_get_level(DSP_IRQ_DETECT_PIN);

#if defined CONFIG_ESP_LYRATD_FT_V1_0_BOARD|| defined CONFIG_ESP_LYRATD_FT_DOSS_V1_0_BOARD
    xSemaphoreGiveFromISR(dsp_asr_sema, 0);
#endif

    xQueueSendFromISR(arg, &val, &xHigherPriorityTaskWoken);
    if (xHigherPriorityTaskWoken != pdFALSE) {
        portYIELD_FROM_ISR();
    }

}

int im501_intr_init(void *queue)
{
    int res = 0;
    gpio_config_t  io_conf;
    memset(&io_conf, 0, sizeof(io_conf));
#ifdef IDF_3_0
    io_conf.intr_type = GPIO_INTR_POSEDGE;
#else
    io_conf.intr_type = GPIO_PIN_INTR_POSEDGE;
#endif
    io_conf.pin_bit_mask = DSP_IRQ_DETECT_PIN_SEL;
    io_conf.mode = GPIO_MODE_INPUT;
    io_conf.pull_down_en = 1;
    io_conf.pull_up_en = 0;
    res |= gpio_config(&io_conf);
    res |= gpio_isr_handler_add(DSP_IRQ_DETECT_PIN, IM501IntrHandler, queue);

    return res;
}

int initial_im501()
{
    unsigned int counter1, counter2;

    esp_err_t ret;
    spi_bus_config_t buscfg = {
        .miso_io_num = DSP_FT_SPI_MISO,
        .mosi_io_num = DSP_FT_SPI_MOSI,
        .sclk_io_num = DSP_FT_SPI_CLK,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1
    };
    spi_device_interface_config_t devcfg = {
        .clock_speed_hz = 10 * 1000 * 1000,     //Clock out at 10 MHz
        .mode = 0,                              //SPI mode 0
        .spics_io_num = DSP_FT_SPI_CS,             //CS pin
        .queue_size = 7,                        //We want to be able to queue 7 transactions at a time
        .pre_cb = NULL, //Specify pre-transfer callback to handle D/C line
    };
    //Initialize the SPI bus
    ret = spi_bus_initialize(HSPI_HOST, &buscfg, 1);
    assert(ret == ESP_OK);
    //Attach the LCD to the SPI bus
    ret = spi_bus_add_device(HSPI_HOST, &devcfg, &im501_spi);
    assert(ret == ESP_OK);

    // //check
    // int val = 0;
    // im501_spi_write_reg(1, 0x07);
    // im501_spi_read_reg(1, &val);
    // ESP_LOGW(IM501_TAG, "vol %x", val);
    // im501_spi_write_reg(1, 0x06);
    // im501_spi_read_reg(1, &val);
    // ESP_LOGW(IM501_TAG, "vol %x", val);
    // im501_spi_write_reg(1, 0x05);
    // im501_spi_read_reg(1, &val);
    // ESP_LOGW(IM501_TAG, "vol %x", val);
    // im501_spi_write_reg(1, 0x04);
    // im501_spi_read_reg(1, &val);
    // ESP_LOGW(IM501_TAG, "vol %x", val);
    // im501_spi_write_reg(1, 0x03);
    // im501_spi_read_reg(1, &val);
    // ESP_LOGW(IM501_TAG, "vol %x", val);


    //test_channel_register();


//    int value = 0x05060708;
//    im501_spi_write_dram(0x0ffe0000, &value);
//    im501_spi_read_dram(0x0ffe0000, &val);
//    if (val != 0x05060708) {
//        ESP_LOGE(IM501_TAG, "D vol %x", val);
//        vTaskDelete(NULL);
//    }

    if (im501_fw_loaded() != 0) {
        ESP_LOGE(IM501_TAG, "im501_fw_loaded failed");
        return -1;
    }

    //check counter
    char read_data[4];
    im501_spi_read_dram(TO_DSP_FRAMECOUNTER_ADDR, read_data);
    counter1 = read_data[3] << 24 | read_data[2] << 16 | read_data[1] << 8 | read_data[0];
    vTaskDelay(50 / portTICK_RATE_MS);
    im501_spi_read_dram(TO_DSP_FRAMECOUNTER_ADDR, read_data);
    counter2 = read_data[3] << 24 | read_data[2] << 16 | read_data[1] << 8 | read_data[0];

    if (counter1 == counter2) {
        ESP_LOGE(IM501_TAG, "%d %d not running", counter2, counter1);
        return -1;
    }

    ESP_LOGI(IM501_TAG, "counter2 %d counter1 %d", counter2, counter1);

    //vTaskDelay(3000);

    // int set_mic_gain(uint16_t mic, uint16_t gain);
    // set_mic_gain(1,0x1800);

    // int set_master_mic(uint16_t mic);
    // set_master_mic(1);

    // int set_volume(uint16_t vol);
    // set_volume(0x400);

    // int send_func_mode0(uint16_t reg);
    // send_func_mode0(0x0010);

    // int close_im501_ffp(void);
    // close_im501_ffp();

    int set_left_channel_PGA(uint16_t gain);
    set_left_channel_PGA(0x900);
    int set_right_channel_PGA(uint16_t gain);
    set_right_channel_PGA(0x900);
    int set_left_channel_audio_process(uint8_t value);
    set_left_channel_audio_process(6);
    int set_right_channel_audio_process(uint8_t value);
    set_right_channel_audio_process(6);

    //test_channel_register();


    return 0;
}

/**
 * [im501_int description]
 * @Author   HengYongchao
 * @DateTime 2018-07-17T17:26:02+0800
 * @version                           [version]
 * @param    arg                      [description]
 */
void im501_int(void *arg)
{
    int queueMessage = 0;
    char read_data[4];
    QueueHandle_t im501xQueue = xQueueCreate(10, sizeof(int));

    if (im501_intr_init(im501xQueue) != 0) {
        ESP_LOGE(IM501_TAG, "im501_intr_init failed");
        vQueueDelete(im501xQueue);
        vTaskDelete(NULL);
    }

    while (1) {
        xQueueReceive(im501xQueue, &queueMessage, portMAX_DELAY);
        im501_irq_handling_work();
        im501_spi_read_dram(TO_DSP_FRAMECOUNTER_ADDR, read_data);
        ESP_LOGI(IM501_TAG, "counter2 : %x %x %x %x", read_data[3], read_data[2], read_data[1], read_data[0]);
    }
    vQueueDelete(im501xQueue);
    vTaskDelete(NULL);
}


/**
 * [set_volume_source description]
 * @Author   HengYongchao
 * @DateTime 2018-07-17T17:25:50+0800
 * @version                           [version]
 * @param    channel                  [description]
 * @param    source                   [description]
 * @return                            [description]
 */
int set_volume_source(uint8_t channel, uint8_t source)
{
    int ret = 0;
    uint16_t val = 0;

    if (channel == LEFT_CHANNEL) {
        if (source == MIC0) {
            val = 0;
            ret = im501_spi_write_dram_2byte(IM501_LEFT_CHANNEL_REG, val);
        } else if (source == MIC1) {
            val = 1;
            ret = im501_spi_write_dram_2byte(IM501_LEFT_CHANNEL_REG, val);
        } else if (source == LINE_OUT) {
            val = 6;
            ret = im501_spi_write_dram_2byte(IM501_LEFT_CHANNEL_REG, val);
        } else {}
    } else if (channel == RIGHT_CHANNEL) {
        if (source == MIC0) {
            val = 0;
            ret = im501_spi_write_dram_2byte(IM501_LEFT_CHANNEL_REG, val);
        } else if (source == MIC1) {
            val = 1;
            ret = im501_spi_write_dram_2byte(IM501_LEFT_CHANNEL_REG, val);
        } else if (source == LINE_OUT) {
            val = 6;
            ret = im501_spi_write_dram_2byte(IM501_LEFT_CHANNEL_REG, val);
        } else {}
    } else {
    }

    return ret;
}


/**
 * [close_im501_ffp description]
 * @Author   HengYongchao
 * @DateTime 2018-07-12T13:48:43+0800
 * @version                           [version]
 * @return                            [description]
 */
int close_im501_ffp(void)
{
    int ret;
    char read_data[4];
    uint8_t write_data[4];
    unsigned int result;

    ret = im501_spi_read_dram(0x0FFFA804, read_data);

    if (ret != 0) {
        return 1;
    }

    result = 0;
    result = read_data[3] << 24 | read_data[2] << 16 | read_data[1] << 8 | read_data[0];
    //ESP_LOGW(IM501_TAG, " FFP_H: %x", (uint16_t)(result >> 16));
    ESP_LOGW(IM501_TAG, " FFP: %x", (uint16_t)(result));

    write_data[2] = read_data[2];
    write_data[3] = read_data[3];
    write_data[0] = 0x78;
    write_data[1] = 0x02;
    ret = im501_spi_write_dram_2byte(0x0FFFA804, 0x0278);

    //ret = im501_spi_write_dram(0x0FFFA804, write_data);
    if (ret != 0) {
        return 1;
    }

    ESP_LOGW(IM501_TAG, " Write FFP: 0x0278");
    memset(read_data, 0x00, sizeof(read_data));
    memset(write_data, 0x00, sizeof(write_data));
    result = 0;
    ret = im501_spi_read_dram(0x0FFFA804, read_data);

    if (ret != 0) {
        return 1;
    }

    result = read_data[3] << 24 | read_data[2] << 16 | read_data[1] << 8 | read_data[0];
    //ESP_LOGW(IM501_TAG, " Read FFP_H again: %x", (uint16_t)(result >> 16));
    ESP_LOGW(IM501_TAG, " Read FFP again: %x\r\n", (uint16_t)(result));

    return 0;
}


/**
 * [SendFuncMode0 description]
 * @Author   HengYongchao
 * @DateTime 2018-07-12T19:01:09+0800
 * @version                           [version]
 * @param    reg                      [SendFuncMode0 ]
 *  +-----------------------------------------------------------------+
 *  | 15|14|13|12  |11|10  |9   |8   |7   |6    |5  |4  |3   |2 |1 |0 |
 *  +------------- +--------------------------------------------------+
 *  |   |  |  |WNS |  |BVE |HPF |DRC |FFP |FdEQ |NS |BF |AEC |  |  |  |
 *  +---+--+-------+-----------------------------------------+--+--+--+
 * @return                            [description]
 */
int send_func_mode0(uint16_t reg)
{
    int ret;
    char read_data[4];
    uint8_t write_data[4];
    unsigned int result;

    ret = im501_spi_read_dram(0x0FFFA804, read_data);

    if (ret != 0) {
        return 1;
    }

    result = 0;
    result = read_data[3] << 24 | read_data[2] << 16 | read_data[1] << 8 | read_data[0];
    //ESP_LOGW(IM501_TAG, " FFP_H: %x", (uint16_t)(result >> 16));
    ESP_LOGW(IM501_TAG, " FuncMode0: %x", (uint16_t)(result));

    write_data[2] = read_data[2];
    write_data[3] = read_data[3];
    write_data[0] = 0x78;
    write_data[1] = 0x02;
    ret = im501_spi_write_dram_2byte(0x0FFFA804, reg);

    //ret = im501_spi_write_dram(0x0FFFA804, write_data);
    if (ret != 0) {
        return 1;
    }

    ESP_LOGW(IM501_TAG, " Write FuncMode0: reg");
    memset(read_data, 0x00, sizeof(read_data));
    memset(write_data, 0x00, sizeof(write_data));
    result = 0;
    ret = im501_spi_read_dram(0x0FFFA804, read_data);

    if (ret != 0) {
        return 1;
    }

    result = read_data[3] << 24 | read_data[2] << 16 | read_data[1] << 8 | read_data[0];
    //ESP_LOGW(IM501_TAG, " Read FFP_H again: %x", (uint16_t)(result >> 16));
    ESP_LOGW(IM501_TAG, " Read FuncMode0 again: %x\r\n", (uint16_t)(result));

    return 0;
}


/**
 * [set_volume description]
 * @Author   HengYongchao
 * @DateTime 2018-07-12T16:12:55+0800
 * @version                           [version]
 * @param    vol                      [default 0x400]
 * @return                            [description]
 */
int set_volume(uint16_t vol)
{
    int ret;
    char read_data[4];
    uint8_t write_data[4];
    unsigned int result;

    ret = im501_spi_read_dram(0x0FFFACF8, read_data);

    if (ret != 0) {
        ESP_LOGI(IM501_TAG, " failed to read 0x0FFFACFA");
        return 1;
    }

    result = read_data[3] << 24 | read_data[2] << 16 | read_data[1] << 8 | read_data[0];
    ESP_LOGW(IM501_TAG, " Read Volume: %x", (uint16_t)(result >> 16));
    //ESP_LOGW(IM501_TAG, " Read Volume L Status: %x", (uint16_t)(result ));

    write_data[0] = read_data[0];
    write_data[1] = read_data[1];
    write_data[2] = (uint8_t)vol;
    write_data[3] = (uint8_t)(vol >> 8);
    ret = im501_spi_write_dram(0x0FFFACF8, write_data);

    if (ret != 0) {
        ESP_LOGI(IM501_TAG, " failed to write 0x0FFFACF8");
        return 1;
    }

    ESP_LOGW(IM501_TAG, " Write Volume: %x", (uint16_t)vol);
    memset(read_data, 0x00, sizeof(read_data));
    memset(write_data, 0x00, sizeof(write_data));
    //ret = im501_spi_write_dram_2byte(0x0FFFBF8A,vol);
    ret = im501_spi_read_dram(0x0FFFACF8, read_data);

    if (ret != 0) {
        ESP_LOGI(IM501_TAG, " failed to read 0x0FFFACF8");
        return 1;
    }

    result = 0;
    result = read_data[3] << 24 | read_data[2] << 16 | read_data[1] << 8 | read_data[0];
    //ESP_LOGW(IM501_TAG, " Read Volume again: %x", (uint16_t)(result ));
    ESP_LOGW(IM501_TAG, " Read Volume again: %x\r\n", (uint16_t)(result >> 16));

    return 0;
}

/**
 * [set_master_mic description]
 * @Author   HengYongchao
 * @DateTime 2018-07-17T17:09:49+0800
 * @version                           [version]
 * @param    mic                      [description]
 * @return                            [description]
 */
int set_master_mic(uint16_t mic)
{
    int ret;
    char read_data[4];
    uint8_t write_data[4];
    unsigned int result = 0;

    if ((mic != 0) && (mic != 1)) {
        ESP_LOGI(IM501_TAG, " Mic para wrong\n");
        return 1;
    }

    ret = im501_spi_read_dram(0x0fffa830, read_data);

    if (ret != 0) {
        return 1;
    }

    result = read_data[3] << 24 | read_data[2] << 16 | read_data[1] << 8 | read_data[0];
    //ESP_LOGW(IM501_TAG, " Read Master Mic: %x", (uint16_t)(result >> 16));
    ESP_LOGW(IM501_TAG, " Read Master Mic Reg: %x", (uint16_t)(result));
    write_data[2] = read_data[2];
    write_data[3] = read_data[3];
    write_data[0] = (uint8_t)mic;
    write_data[1] = (uint8_t)(mic >> 8);
    ret = im501_spi_write_dram_2byte(0x0fffa830, mic);
    ret = im501_spi_write_dram(0x0fffa830, write_data);

    if (ret != 0) {
        return 1;
    }

    ESP_LOGW(IM501_TAG, " Write Master Mic Reg: %x", (uint16_t)mic);
    memset(read_data, 0x00, sizeof(read_data));
    memset(write_data, 0x00, sizeof(write_data));
    ret = im501_spi_read_dram(0x0fffa830, read_data);

    if (ret != 0) {
        return 1;
    }

    result = 0;
    result = read_data[3] << 24 | read_data[2] << 16 | read_data[1] << 8 | read_data[0];
    //ESP_LOGW(IM501_TAG, " Read Master Mic again: %x\r\n", (uint16_t)(result >> 16));
    ESP_LOGW(IM501_TAG, " Read Master Mic Reg again : %x\r\n", (uint16_t)(result));
    return 0;

}


/**
 * [set_left_channel_PGA description]
 * @Author   HengYongchao
 * @DateTime 2018-07-11T15:46:09+0800
 * @version                           [version]
 * @param    gain                     [This is a hexadecimal parameter ]
 * @return                            [description]
 */
int set_left_channel_PGA(uint16_t gain)
{
    int ret;
    char read_data[4];
    uint8_t write_data[4];
    unsigned int result = 0;

    if (gain > 0x7fff) {
        ESP_LOGI(IM501_TAG, " Gain must less than 0x7fff\n");
        return 1;
    }

    //ESP_LOGI(IM501_TAG, " Set left channel pga  ");
    ret = im501_spi_read_dram(0x0fffa830, read_data);

    if (ret != 0) {
        return 1;
    }

    result = read_data[3] << 24 | read_data[2] << 16 | read_data[1] << 8 | read_data[0];
    ESP_LOGW(IM501_TAG, " Read Left Channel PGA_0: %x", (uint16_t)(result >> 16));

    write_data[0] = read_data[0];
    write_data[1] = read_data[1];
    write_data[2] = (uint8_t)gain;
    write_data[3] = (uint8_t)(gain >> 8);
    //ret = im501_spi_write_dram_2byte(0x0fffa832,gain);
    ret = im501_spi_write_dram(0x0fffa830, write_data);

    if (ret != 0) {
        return 1;
    }

    ESP_LOGW(IM501_TAG, " Write PGA_0: %x", (uint16_t)gain);
    memset(read_data, 0x00, sizeof(read_data));
    memset(write_data, 0x00, sizeof(write_data));
    ret = im501_spi_read_dram(0x0fffa830, read_data);

    if (ret != 0) {
        return 1;
    }

    result = 0;
    result = read_data[3] << 24 | read_data[2] << 16 | read_data[1] << 8 | read_data[0];
    ESP_LOGW(IM501_TAG, " Read Left Channel PGA_0 again: %x\r\n", (uint16_t)(result >> 16));
    //ESP_LOGW(IM501_TAG, " Read Left Channel PGA_0 again: %x", (uint16_t)(result ));
    return 0;

}

/**
 * [set_right_channel_PGA description]
 * @Author   HengYongchao
 * @DateTime 2018-07-11T15:46:27+0800
 * @version                           [version]
 * @param    gain                     [This is a hexadecimal parameter ]
 * @return                            [description]
 */
int set_right_channel_PGA(uint16_t gain)
{
    int ret;
    char read_data[4];
    uint8_t write_data[4];
    unsigned int result;

    if (gain > 0x7fff) {
        ESP_LOGI(IM501_TAG, " Gain must less than 0x7fff\n");
        return 1;
    }

    ret = im501_spi_read_dram(0x0fffa834, read_data);

    if (ret != 0) {
        return 1;
    }

    result = read_data[3] << 24 | read_data[2] << 16 | read_data[1] << 8 | read_data[0];
    ESP_LOGW(IM501_TAG, " Read Right Channel PGA_1: %x", (uint16_t)result);

    write_data[2] = read_data[2];
    write_data[3] = read_data[3];
    write_data[0] = (uint8_t)gain;
    write_data[1] = (uint8_t)(gain >> 8);
    ret = im501_spi_write_dram_2byte(0x0fffa834, gain);

    //ret = im501_spi_write_dram(0x0fffa834, write_data);
    if (ret != 0) {
        return 1;
    }

    ESP_LOGW(IM501_TAG, " Write PGA_0: %x", (uint16_t)gain);
    memset(read_data, 0x00, sizeof(read_data));
    memset(write_data, 0x00, sizeof(write_data));
    ret = im501_spi_read_dram(0x0fffa834, read_data);

    if (ret != 0) {
        return 1;
    }

    result = 0;
    result = read_data[3] << 24 | read_data[2] << 16 | read_data[1] << 8 | read_data[0];
    //ESP_LOGW(IM501_TAG, " Read Left Channel PGA_0 H again: %x", (uint16_t)(result>>16));
    ESP_LOGW(IM501_TAG, " Read Left Channel PGA_0 again: %x\r\n", (uint16_t)result);
    return 0;
}


/**
 * [set_left_channel_audio_process include AEC,NS,BF]
 * @Author   HengYongchao
 * @DateTime 2018-07-11T16:32:30+0800
 * @version                           [version]
 * @param    value                    [Only 0,1,6]
 * @return                            [description]
 */
int set_left_channel_audio_process(uint8_t value)
{
    int ret;
    char read_data[4];
    uint8_t write_data[4];
    unsigned int result;

    if ((value != 0) && (value != 1) && (value != 6)) {
        ESP_LOGI(IM501_TAG, " Bad parameter of set_left_channel_audio_process,Only 0,1,6 acceptable");
        return 1;
    }

    ret = im501_spi_read_dram(0x0FFFBF88, read_data);

    if (ret != 0) {
        ESP_LOGI(IM501_TAG, " failed to read 0x0FFFBF88");
        return 1;
    }

    result = read_data[3] << 24 | read_data[2] << 16 | read_data[1] << 8 | read_data[0];
    ESP_LOGW(IM501_TAG, " Read Left Channel Status: %x", (uint16_t)(result >> 16));

    write_data[0] = read_data[0];
    write_data[1] = read_data[1];
    write_data[2] = (uint8_t)value;
    write_data[3] = 0;
    ret = im501_spi_write_dram(0x0FFFBF88, write_data);

    if (ret != 0) {
        ESP_LOGI(IM501_TAG, " failed to write 0x0FFFBF88");
        return 1;
    }

    ESP_LOGW(IM501_TAG, " Write Left Channel: %x", (uint16_t)value);
    memset(read_data, 0x00, sizeof(read_data));
    memset(write_data, 0x00, sizeof(write_data));
    //ret = im501_spi_write_dram_2byte(0x0FFFBF8A,value);
    ret = im501_spi_read_dram(0x0FFFBF88, read_data);

    if (ret != 0) {
        ESP_LOGI(IM501_TAG, " failed to read 0x0FFFBF88");
        return 1;
    }

    result = 0;
    result = read_data[3] << 24 | read_data[2] << 16 | read_data[1] << 8 | read_data[0];
    ESP_LOGW(IM501_TAG, " Read Left Channel Status H again: %x\r\n", (uint16_t)(result >> 16));
    //ESP_LOGW(IM501_TAG, " Read Left Channel Status again: %x", (uint16_t)(result ));
    return 0;
}


/**
 * [set_right_channel_audio_process include AEC,NS,BF]
 * @Author   HengYongchao
 * @DateTime 2018-07-11T16:32:53+0800
 * @version                           [version]
 * @param    value                    [Only 0,1,6]
 * @return                            [description]
 */
int set_right_channel_audio_process(uint8_t value)
{
    int ret;
    char read_data[4];
    uint8_t write_data[4];
    unsigned int result;

    if ((value != 0) && (value != 1) && (value != 6)) {
        ESP_LOGI(IM501_TAG, " Bad parameter of set_right_channel_audio_process,Only 0,1,6 acceptable");
        return 1;
    }

    ret = im501_spi_read_dram(0x0FFFBF8C, read_data);

    if (ret != 0) {
        ESP_LOGI(IM501_TAG, " failed to read 0x0FFFBF8C");
        return 1;
    }

    result = read_data[3] << 24 | read_data[2] << 16 | read_data[1] << 8 | read_data[0];
    ESP_LOGW(IM501_TAG, " Read Right Channel Status: %x", (uint16_t)result);

    write_data[2] = read_data[2];
    write_data[3] = read_data[3];
    write_data[0] = (uint8_t)value;
    write_data[1] = 0;
    ret = im501_spi_write_dram(0x0FFFBF8C, write_data);

    if (ret != 0) {
        ESP_LOGI(IM501_TAG, " failed to write 0x0FFFBF8C");
        return 1;
    }

    ESP_LOGW(IM501_TAG, " Write Right Channel: %x", (uint16_t)value);
    memset(read_data, 0x00, sizeof(read_data));
    memset(write_data, 0x00, sizeof(write_data));
    //ret = im501_spi_write_dram_2byte(0x0FFFBF8C,value);
    ret = im501_spi_read_dram(0x0FFFBF8C, read_data);

    if (ret != 0) {
        ESP_LOGI(IM501_TAG, " failed to write 0x0FFFBF8C");
        return 1;
    }

    result = 0;
    result = read_data[3] << 24 | read_data[2] << 16 | read_data[1] << 8 | read_data[0];
    ESP_LOGW(IM501_TAG, " Read Left Channel Status again: %x", (uint16_t)result);
    //ESP_LOGW(IM501_TAG, " Read Left Channel H Status again: %x\r\n", (uint16_t)(result>>16));

    return 0;
}


/**
 * [set_mic_gain description]
 * @Author   HengYongchao
 * @DateTime 2018-07-17T18:32:19+0800
 * @version                           [version]
 * @param    mic                      [support two mic,mic0,mic1]
 * @param    gain                     [description]
 * @return                            [description]
 */
int set_mic_gain(uint16_t mic, uint16_t gain)
{
    send_func_mode0(0);
    set_master_mic(mic);
    set_left_channel_PGA(gain);
    set_right_channel_PGA(gain);
    set_left_channel_audio_process(6);
    set_right_channel_audio_process(6);

    return 0;
}


/**
 * [test_channel_register description]
 * @Author   HengYongchao
 * @DateTime 2018-07-17T17:10:15+0800
 * @version                           [version]
 */
void test_channel_register(void)
{
    char read_data[4];
    uint8_t write_data[4];
    unsigned int counter1, counter2;
    ESP_LOGW(IM501_TAG, " *********************************************************************** ");

    im501_spi_read_dram(0x0fffa830, read_data);
    counter1 = read_data[3] << 24 | read_data[2] << 16 | read_data[1] << 8 | read_data[0];
    ESP_LOGW(IM501_TAG, " PGA_0 %x", (uint16_t)(counter1 >> 16));
    //ESP_LOGW(IM501_TAG, " PGA_0 %x", (uint16_t)counter1 );

    write_data[0] = read_data[0];
    write_data[1] = read_data[1];
    write_data[2] = 0x00;
    write_data[3] = 0x18;
    im501_spi_write_dram(0x0fffa830, write_data);
    //ESP_LOGW(IM501_TAG, "0x0fffa832, -> 0X1800");
    im501_spi_read_dram(0x0fffa830, read_data);
    counter1 = read_data[3] << 24 | read_data[2] << 16 | read_data[1] << 8 | read_data[0];
    ESP_LOGW(IM501_TAG, " PGA_0 %x", (uint16_t)(counter1 >> 16));

    im501_spi_read_dram(0x0FFFBF88, read_data);
    counter1 = read_data[3] << 24 | read_data[2] << 16 | read_data[1] << 8 | read_data[0];
    //ESP_LOGW(IM501_TAG, " left %x", (uint16_t)counter1);
    ESP_LOGW(IM501_TAG, " LEFT(0x0FFFBF8A): %x", (uint16_t)(counter1 >> 16));

    im501_spi_read_dram(0x0FFFBF8C, read_data);
    counter1 = read_data[3] << 24 | read_data[2] << 16 | read_data[1] << 8 | read_data[0];
    ESP_LOGW(IM501_TAG, " RIGHT(0x0FFFBF8C): %x", (uint16_t)counter1);
    //ESP_LOGW(IM501_TAG, " right %x", (uint16_t)(counter1 >> 16));


    ESP_LOGW(IM501_TAG, " *********************************************************************** ");

}


#endif
