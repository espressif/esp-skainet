#include "tts_urat.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/fcntl.h>
#include <sys/errno.h>
#include <sys/unistd.h>
#include <sys/select.h>

#include "ringbuf.h"
#include "esp_log.h"
#include "driver/uart.h"
#include "soc/uart_periph.h"
#include "esp_idf_version.h"

#define TAG "TTS_URAT"
extern ringbuf_handle_t urat_rb;

void uartTask(void *arg)
{
    uart_config_t uart_config = {
        .baud_rate = 115200,
        .data_bits = UART_DATA_8_BITS,
        .parity    = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
#if ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(5, 0, 0)
        .source_clk = UART_SCLK_DEFAULT,
#endif
    };
    uart_param_config(UART_NUM_0, &uart_config);
    uart_driver_install(UART_NUM_0, 2*URAT_BUF_LEN, 0, 0, NULL, 0);
    char data[URAT_BUF_LEN];
    int len=0;

    while (1) {
        int fd;

        if ((fd = open("/dev/uart/0", O_RDWR)) == -1) {
            ESP_LOGE(TAG, "Cannot open UART");
            vTaskDelay(5000 / portTICK_PERIOD_MS);
            continue;
        }

        // We have a driver now installed so set up the read/write functions to use driver also.
        esp_vfs_dev_uart_use_driver(0);

        while (1) {
            int s;
            fd_set rfds;
            struct timeval tv = {
                .tv_sec = 5,
                .tv_usec = 0,
            };

            FD_ZERO(&rfds);
            FD_SET(fd, &rfds);

            s = select(fd + 1, &rfds, NULL, NULL, &tv);

            if (s < 0) {
                ESP_LOGE(TAG, "Select failed: errno %d", errno);
                break;
            } else if (s == 0) {
                continue;
            } else {
                if (FD_ISSET(fd, &rfds)) {
                    char buf;
                    if (read(fd, &buf, 1) > 0) {
                        ESP_LOGI("", "%c", buf);
                        rb_write(urat_rb, &buf, 1, portMAX_DELAY);

                        if (buf=='\n') {
                            data[len]='\0';
                            printf("uart input: %s\n", data);
                            len=0;
                        } else {
                            data[len]=buf;
                            len++;
                        }
                        // Note: Only one character was read even the buffer contains more. The other characters will
                        // be read one-by-one by subsequent calls to select() which will then return immediately
                        // without timeout.
                    } else {
                        ESP_LOGE(TAG, "UART read error");
                        break;
                    }
                } else {
                    ESP_LOGE(TAG, "No FD has been set in select()");
                    break;
                }
            }
        }

        close(fd);
    }

    vTaskDelete(NULL);
}