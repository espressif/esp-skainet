#include "uart.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/fcntl.h>
#include <sys/errno.h>
#include <sys/unistd.h>
#include <sys/select.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "ringbuf.h"
#include "esp_log.h"
#include "sdkconfig.h"
#include "esp_idf_version.h"

#if defined(CONFIG_ESP_CONSOLE_USB_SERIAL_JTAG)
#include "driver/usb_serial_jtag.h"
#include "esp_vfs_dev.h"
#else
#include "driver/uart.h"
#include "driver/uart_vfs.h"
#endif

#define TAG "TTS_UART"

static void handle_rx_byte(ringbuf_handle_t uart_rb, char buf, char *data, int *len)
{
    rb_write(uart_rb, &buf, 1, portMAX_DELAY);

    if (buf == '\n') {
        data[*len] = '\0';
        printf("uart input: %s\n", data);
        fflush(stdout);
        *len = 0;
    } else if (*len < UART_BUF_LEN - 1) {
        data[*len] = buf;
        (*len)++;
    } else {
        ESP_LOGE(TAG, "RX line too long, drop");
        *len = 0;
    }
}

#if defined(CONFIG_ESP_CONSOLE_USB_SERIAL_JTAG)
/*
 * ESP32-P4 Function EV uses USB Serial/JTAG as the primary console.
 * Use the interrupt-driven driver so host writes are drained into its RX
 * ring buffer instead of being limited by the small hardware FIFO.
 */
static void usb_serial_jtag_read_loop(ringbuf_handle_t uart_rb)
{
    char data[UART_BUF_LEN];
    int len = 0;
    int fd = fileno(stdin);
    usb_serial_jtag_driver_config_t config = USB_SERIAL_JTAG_DRIVER_CONFIG_DEFAULT();

    ESP_ERROR_CHECK(usb_serial_jtag_driver_install(&config));
    esp_vfs_usb_serial_jtag_use_driver();

    /* Ensure host tests see command echoes immediately. */
    setvbuf(stdout, NULL, _IONBF, 0);
    setvbuf(stdin, NULL, _IONBF, 0);

    int flags = fcntl(fd, F_GETFL, 0);
    if (flags >= 0) {
        fcntl(fd, F_SETFL, flags | O_NONBLOCK);
    }

    ESP_LOGI(TAG, "Read control commands from USB Serial/JTAG stdin");

    while (1) {
        char buf;
        int n = read(fd, &buf, 1);
        if (n > 0) {
            handle_rx_byte(uart_rb, buf, data, &len);
        } else {
            vTaskDelay(pdMS_TO_TICKS(10));
        }
    }
}
#else
static void uart0_read_loop(ringbuf_handle_t uart_rb)
{
    char data[UART_BUF_LEN];
    int len = 0;
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
    uart_driver_install(UART_NUM_0, 2 * UART_BUF_LEN, 0, 0, NULL, 0);

    while (1) {
        int fd;

        if ((fd = open("/dev/uart/0", O_RDWR)) == -1) {
            ESP_LOGE(TAG, "Cannot open /dev/uart/0: errno %d", errno);
            vTaskDelay(5000 / portTICK_PERIOD_MS);
            continue;
        }

        uart_vfs_dev_use_driver(0);

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
            } else if (FD_ISSET(fd, &rfds)) {
                char buf;
                if (read(fd, &buf, 1) > 0) {
                    handle_rx_byte(uart_rb, buf, data, &len);
                } else {
                    ESP_LOGE(TAG, "UART read error");
                    break;
                }
            } else {
                ESP_LOGE(TAG, "No FD has been set in select()");
                break;
            }
        }

        close(fd);
    }
}
#endif

void uart_read_task(void *arg)
{
    ringbuf_handle_t uart_rb = (ringbuf_handle_t) arg;

#if defined(CONFIG_ESP_CONSOLE_USB_SERIAL_JTAG)
    usb_serial_jtag_read_loop(uart_rb);
#else
    uart0_read_loop(uart_rb);
#endif

    vTaskDelete(NULL);
}
