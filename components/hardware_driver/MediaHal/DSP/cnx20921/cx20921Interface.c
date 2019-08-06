/*
*
* Copyright 2015-2016 Espressif Systems (Shanghai) PTE LTD
*
* Licensed under the Apache License, Version 2.0 (the "License");
* you may not use this file except in compliance with the License.
* You may obtain a copy of the License at
*
*     http://www.apache.org/licenses/LICENSE-2.0
*
* Unless required by applicable law or agreed to in writing, software
* distributed under the License is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
* See the License for the specific language governing permissions and
* limitations under the License.
*
*/

/*  ----------------------------------------------------------------------------------------
*   |                                                                                       |
*   |   The file includes functions and variables to configure CX20921.                     |
*   |                                                                                       |
*   ----------------------------------------------------------------------------------------
*/
#include <string.h>
#include "esp_log.h"
#include "esp_partition.h"
#include "driver/gpio.h"
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>
#include "esp_spiffs.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"

#include "cx20921_i2c.h"
#include "cx20921Interface.h"
#include "CxFlash.h"
#include "esp_audio_mem.h"

#define CX20921_TAG "CX20921"

#define BOOTLOADER_BIN "/spiffs/bootloader.bin"
#define ALEXA_FW "/spiffs/alexa.sfs"

#define CX2091_ASSERT(a, format, b, ...) \
    if ((a) != 0) { \
        ESP_LOGE(CX20921_TAG, format, ##__VA_ARGS__); \
        return b;\
    }
#define CX2091_CHECK_NULL(a, format, b, ...) \
        if ((a) == NULL) { \
            ESP_LOGE(CX20921_TAG, format, ##__VA_ARGS__); \
            return b;\
        }

#define CX2091_LOG(fmt, ...)   ESP_LOGI(CX20921_TAG, fmt, ##__VA_ARGS__)

static void IRAM_ATTR cx20921IntrHandler(void *arg)
{
    xQueueHandle queue = (xQueueHandle) arg;
    gpio_num_t gpioNum = (gpio_num_t) GPIO_CX20921_IRQ;
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    xQueueSendFromISR(queue, &gpioNum, &xHigherPriorityTaskWoken);
    if (xHigherPriorityTaskWoken) {
        portYIELD_FROM_ISR();
    }
}

static esp_err_t spiffs_init()
{
    ESP_LOGI(CX20921_TAG, "Initializing SPIFFS");

    esp_vfs_spiffs_conf_t conf = {
      .base_path = "/spiffs",
      .partition_label = NULL,
      .max_files = 10,
      .format_if_mount_failed = false
    };

    esp_err_t ret = esp_vfs_spiffs_register(&conf);
    if (ret != ESP_OK) {
        if (ret == ESP_FAIL) {
            ESP_LOGE(CX20921_TAG, "Failed to mount or format filesystem. Did you flash cnx_fw.spiffs on ESP32?");
        } else if (ret == ESP_ERR_NOT_FOUND) {
            ESP_LOGE(CX20921_TAG, "Failed to find SPIFFS partition");
        } else {
            // ESP_LOGE(CX20921_TAG, "Failed to initialize SPIFFS (%s)", esp_err_to_name(ret));
        }
    }
    size_t total = 0, used = 0;
    ret = esp_spiffs_info(NULL, &total, &used);
    if (ret != ESP_OK) {
        // ESP_LOGE(CX20921_TAG, "Failed to get SPIFFS partition information (%s)", esp_err_to_name(ret));
        } else {
        ESP_LOGI(CX20921_TAG, "Partition size: total: %d, used: %d", total, used);
    }

    return ret;
}

static esp_err_t spiffs_load_fw(int img, uint8_t **outdata, size_t *outsize)
{
    FILE* file;
    size_t fw_size;
    size_t read_bytes;
    struct stat siz =  { 0 };
    if (img == 0) {
        file = fopen(BOOTLOADER_BIN, "r");
        if (file == NULL) {
            ESP_LOGE(CX20921_TAG, "Failed to open bootloader");
            return ESP_FAIL;
        }
        if (stat(BOOTLOADER_BIN, &siz) == -1) {
            ESP_LOGE(CX20921_TAG, "Error");
            fclose(file);
            return ESP_FAIL;
        }
    } else {
        file = fopen(ALEXA_FW, "r");
        if (file == NULL) {
            ESP_LOGE(CX20921_TAG, "Failed to open alexa firmware");
            return ESP_FAIL;
        }
        if (stat(ALEXA_FW, &siz) == -1) {
            ESP_LOGE(CX20921_TAG, "Error");
            fclose(file);
            return ESP_FAIL;
        }
    }

    fw_size = siz.st_size;
    *outsize = fw_size;
    *outdata = esp_audio_mem_calloc(1, fw_size);
    if (!*outdata) {
        printf("Error allocating data");
        fclose(file);
        return ESP_ERR_NO_MEM;
    }
    read_bytes = fread(*outdata, 1, fw_size, file);
    if (read_bytes == -1) {
        ESP_LOGE(CX20921_TAG, "Reading firmware failed");
        free(*outdata);
        fclose(file);
        return ESP_FAIL;
    }
    printf("read data %x fw_size %d read_bytes %d", *outdata[0], fw_size, read_bytes);
    fclose(file);
    return ESP_OK;
}

int cx20921GpioInit()
{
    int res = 0;
    gpio_config_t  io_conf;
    memset(&io_conf, 0, sizeof(io_conf));
    io_conf.intr_type = GPIO_PIN_INTR_DISABLE;
    io_conf.mode = GPIO_MODE_OUTPUT;
    io_conf.pin_bit_mask = GPIO_CX20921_RESET_SEL;
    io_conf.pull_down_en = 0;
    io_conf.pull_up_en = 0;
    res |= gpio_config(&io_conf);
    res |= gpio_set_level(GPIO_CX20921_DSP_RESET, 0);
    vTaskDelay(100 / portTICK_RATE_MS);
    res |= gpio_set_level(GPIO_CX20921_DSP_RESET, 1);
    return res;
}


int cx20921IntrIoGet()
{
    vTaskDelay(100 / portTICK_RATE_MS);
    return gpio_get_level(GPIO_CX20921_IRQ);
}

int cx20921IntrInit(void *queue)
{
    int res = 0;
    gpio_config_t  io_conf;
    memset(&io_conf, 0, sizeof(io_conf));
    io_conf.intr_type = GPIO_INTR_POSEDGE;
    io_conf.pin_bit_mask = GPIO_CX20921_IRQ_SEL;
    io_conf.mode = GPIO_MODE_INPUT;
    io_conf.pull_down_en = 0;
    io_conf.pull_up_en = 0;
    res |= gpio_config(&io_conf);
    res |= gpio_isr_handler_add(GPIO_CX20921_IRQ, cx20921IntrHandler, (xQueueHandle *) queue);

    return res;
}



/**
 * @brief Initialize the cx2091 DSP
 *
 * @param reflash  a flag indicating whether to reflash the flash used by DSP,
 *                  two different binaries must be flashed, one of which is for boot while another is a .sys file,
 *                  the two binaries are stored in different flash areas indicated by the partition table in 'partitions_esp_audio.csv'.
 *
 * @return
 *     - (-1) Failed
 *     - ( 0) Succeed
 */
int cx20921Init(int reflash)
{
#define INTERNAL_SIZE (3 * 1024)
    esp_err_t ret = 0;

    SetupI2cWriteMemCallback( (void *) NULL, (fn_I2cWriteMem) i2c_write_imp, DEF_MAX_I2C_WRITE_LEN);
    SetupI2cReadMemCallback( (void *) NULL, (fn_I2cReadMem) i2c_read_imp, DEF_MAX_I2C_READ_LEN);
    SetupSetResetPin((void *) NULL, (fn_SetResetPin) SetResetPin);

    vTaskDelay(1000 / portTICK_RATE_MS);
    cx20921SetVolume(500);
    vTaskDelay(1000 / portTICK_RATE_MS);
    cx20921SetMicGain(60);

    //download firmware
    if (reflash)
    {
        uint8_t *loader = NULL;
        uint8_t *img = NULL;
        size_t loader_size = 0;
        size_t img_size = 0;

        ret = spiffs_init();
        if (ret != ESP_OK) {
            return -1;
        }
        ret = spiffs_load_fw(0, &loader, &loader_size);
        if (ret != ESP_OK) {
            return -1;
        }
        ESP_LOGI(CX20921_TAG, "load bootloader ok");

        ret = spiffs_load_fw(1, &img, &img_size);
        if (ret != ESP_OK) {
            free(loader);
            return -1;
        }
        ESP_LOGI(CX20921_TAG, "load fw ok");

        char *buf = esp_audio_mem_calloc(1, GetSizeOfBuffer());
        if (!buf) {
            printf("Error in allocating buf\n");
            free(loader);
            free(img);
            return -1;
        }

        //
        //  Download FW.
        //
        //If the operation completes successfull,the return value is zero. Otherwise,
        //return EERON_* error code. For more information about return code, please refer
        //to cxpump.h file.
        ret |= DownloadFW(buf, loader, (uint32_t)loader_size, img, (uint32_t)img_size, CX20921_I2C_ADDR >> 1,
                          SFS_UPDATE_AUTO, 1, 0);

        free(buf);
        free(img);
        free(loader);
        CX2091_ASSERT(ret, "reflash", -1);
    }

    //init
    CX2091_ASSERT(DspI2cInit(), "DspI2cInit", -1);
    CX2091_ASSERT(cx20921GpioInit(), "cx20921GpioInit", -1);

    if (ret != 0 )
    {
        ESP_LOGE(CX20921_TAG, "cx20921Init failed %d", ret);
    }

    return ret;
}

int cx20921ReadReg(int reg, int *data)
{
    int res = 0;
    *data = 0;

    res = DspI2cReadReg(CX20921_I2C_ADDR, (uint8_t *)&reg, 2, (uint8_t *)data, 2);
    ESP_LOGI(CX20921_TAG, "read res %d data %d", res, *data);

    return res;
}

int cx20921WriteReg(int reg, int data)
{
    int res = 0;

    res = DspI2cWriteReg(CX20921_I2C_ADDR, (uint8_t *)&reg, 2, (uint8_t *)&data, 2);
    ESP_LOGI(CX20921_TAG, "write res %d data %d", res, data);

    return res;
}


int cx20921SetChannel(Cx2091Channel channel)
{
    int res = -1;
    int argv[4] = {0xD308632C, 0x40, 0xc0, 0xc0};

    switch (channel) {
        case CX2091_CHANNEL_PROCESS:
            res = cxdish_sendcmd(4, argv);
            break;
        case CX2091_CHANNEL_REF:
            argv[2] = 0x02c0;
            argv[3] = 0x02c1;
            res = cxdish_sendcmd(4, argv);
            break;
        case CX2091_CHANNEL_MIC:
            argv[2] = 0x01c0;
            argv[3] = 0x01c1;
            res = cxdish_sendcmd(4, argv);
            break;
        case CX2091_CHANNEL_MIC_PROCESS:
            argv[2] = 0x01c0;
            res = cxdish_sendcmd(4, argv);
            break;
        default:
            ESP_LOGE(CX20921_TAG, "invalid param %d", channel);
            break;
    }

    return res;
}

int cx20921SetMicGain(int vol)
{
    int res = -1;
    int argv[4] = {0xB72D3300, 0x0000030, 0x0000000e, 00000030};//ELBA_ADC0_BOOST

    if (vol > 0x3c) {
        vol = 0x3c;    // 30db
    } else if (vol < 0) {
        vol = 0;    // 0db
    }
    argv[3] = vol;
    res = cxdish_sendcmd(4, argv);
    argv[2] = 0x0000000f;//ELBA_ADC1_BOOST
    res |= cxdish_sendcmd(4, argv);

    return res;
}

int cx20921GetMicGain()
{
    int res = -1;
    int argv[3] = {0xB72D3300, 0x00000130, 0x0000000e};//ELBA_ADC0_BOOST

    res = cxdish_sendcmd(3, argv);
    argv[2] = 0x0000000f;//ELBA_ADC1_BOOST
    res |= cxdish_sendcmd(3, argv);

    return res;
}

int cx20921SetVolume(int vol)
{
    int res = -1;
    int argv[4] = {0xB72D3300, 0x0000030, 0x00000006, 00000030};//ELBA_ADC0_GAIN

    if (vol > 0x2b8) {
        vol = 0x2b8;    // 36db
    } else if (vol < 0) {
        vol = 0;    // -51db
    }
    argv[3] = vol;
    res = cxdish_sendcmd(4, argv);
    argv[2] = 0x00000007;//ELBA_ADC1_GAIN
    res |= cxdish_sendcmd(4, argv);

    return res;
}

int cx20921GetVolume()
{
    int res = -1;
    int argv[3] = {0xB72D3300, 0x00000130, 0x00000006};//ELBA_ADC0_BOOST

    res = cxdish_sendcmd(3, argv);
    argv[2] = 0x00000007;//ELBA_ADC1_BOOST
    res |= cxdish_sendcmd(3, argv);

    return res;
}

