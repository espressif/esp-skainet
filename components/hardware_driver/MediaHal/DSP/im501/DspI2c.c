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

#include <string.h>
#include "esp_log.h"
#include "i2c_bus.h"
#include "DspI2c.h"
#include "userconfig.h"

#define DSPI2C_TAG "DSP_I2C"


#define DSP_I2C_ASSERT(a, format, b, ...) \
    if ((a) != 0) { \
        ESP_LOGE(DSPI2C_TAG, format, ##__VA_ARGS__); \
        return b;\
    }

#define LOG_DSPI2C(fmt, ...)   ESP_LOGI(DSPI2C_TAG, fmt, ##__VA_ARGS__)


static i2c_config_t i2c_cfg = {
    .mode = I2C_MODE_MASTER,
    .sda_io_num = IIC_DATA,
    .scl_io_num = IIC_CLK,
    .sda_pullup_en = GPIO_PULLUP_ENABLE,
    .scl_pullup_en = GPIO_PULLUP_ENABLE,
    .master.clk_speed = 400000
};

int DspI2cWriteReg(int addr, uint8_t* reg, int regLlen, uint8_t* data, int dataLen)
{
    int res = 0;
    res |= i2c_bus_write_bytes(0, addr, reg, regLlen, data, dataLen);
    DSP_I2C_ASSERT(res, "DspI2cWriteReg error", -1);

    return res;
}

int DspI2cWrite(int addr, uint8_t* data, int dataLen)
{
    int res = 0;
    res |= i2c_bus_write_data(0, addr, data, dataLen);
    DSP_I2C_ASSERT(res, "DspI2cWriteReg error", -1);

    return res;
}

int DspI2cReadReg(int addr, uint8_t* reg, int regLlen, uint8_t* output, int dataLen)
{
    int res = 0;
    res |= i2c_bus_write_data(0, addr, reg, regLlen);
    res |= i2c_bus_read_bytes(0, addr | 0x01, output, dataLen);
    DSP_I2C_ASSERT(res, "DspI2cReadReg error", -1);
    return res;
}

int DspI2cRead(int addr, uint8_t* output, int dataLen)
{
    int res = 0;
    res |= i2c_bus_read_bytes(0, addr | 0x01, output, dataLen);
    DSP_I2C_ASSERT(res, "DspI2cReadReg error", -1);
    return res;
}

int DspI2cInit(void)
{
    int res = 0;


    if (i2c_bus_create(0, &i2c_cfg) == NULL) {
        res = -1;
    }

    return res;
}



