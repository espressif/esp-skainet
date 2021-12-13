/*
   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/i2s.h"
#include <driver/gpio.h>

// #include "ie_kaiji.h"
#include "m_0.h"
#include "m_1.h"
#include "m_2.h"
#include "m_3.h"
#include "m_4.h"
#include "m_5.h"
#include "m_6.h"
#include "m_7.h"
#include "m_8.h"
#include "m_9.h"
#include "m_10.h"
#include "m_11.h"
#include "m_12.h"
#include "m_13.h"
#include "m_14.h"
#include "m_15.h"
#include "m_16.h"
#include "m_17.h"
#include "wake_up_prompt_tone.h"
#include "speech_commands_action.h"

extern int detect_flag;

typedef struct {
    char* name;
    const uint16_t* data;
    int length;
} dac_audio_item_t;

#if defined CONFIG_ESP32_S3_KORVO_V1_0_BOARD || defined CONFIG_ESP32_S3_KORVO_V2_0_BOARD || defined CONFIG_ESP32_S3_KORVO_V3_0_BOARD || defined CONFIG_ESP32_S3_KORVO_V4_0_BOARD
#include "led_strip.h"
#include "driver/rmt.h"
#define EXAMPLE_CHASE_SPEED_MS (10)
#define RMT_TX_CHANNEL RMT_CHANNEL_0
led_strip_t *strip = NULL;
void led_Task(void *arg)
{
    int start_led = 1;
    uint32_t red = 0;
    uint32_t green = 0;
    uint32_t blue = 0;
    uint16_t hue = 0;
    uint16_t start_rgb = 0;
    rmt_config_t config = RMT_DEFAULT_CONFIG_TX(19, RMT_TX_CHANNEL);
    // set counter clock to 40MHz
    config.clk_div = 2;

    ESP_ERROR_CHECK(rmt_config(&config));
    ESP_ERROR_CHECK(rmt_driver_install(config.channel, 0, 0));

    // install ws2812 driver
    led_strip_config_t strip_config = LED_STRIP_DEFAULT_CONFIG(12, (led_strip_dev_t)config.channel);
    strip = led_strip_new_rmt_ws2812(&strip_config);
    if (!strip) {
        ets_printf("install WS2812 driver failed");
    }
    // Clear LED strip (turn off all LEDs)
    ESP_ERROR_CHECK(strip->clear(strip, 100));
    for (int j = 0; j < 12; j += 1) {
        ESP_ERROR_CHECK(strip->set_pixel(strip, j, 50, 50, 50));
    }
    // Flush RGB values to LEDs
    ESP_ERROR_CHECK(strip->refresh(strip, 100));
    while (1) {
        for (int i = 0; i < 100; i++) {
            for (int j = 0; j < 12; j += 1) {
                // Build RGB values
                ESP_ERROR_CHECK(strip->set_pixel(strip, j, 100 * detect_flag, 0.5 * i * 0, 0.5 * i * (1 - detect_flag)));
                // Flush RGB values to LEDs
                ESP_ERROR_CHECK(strip->refresh(strip, 100));
            }
            vTaskDelay(pdMS_TO_TICKS(EXAMPLE_CHASE_SPEED_MS));
        }

        for (int i = 100; i > 0; i--) {
            for (int j = 0; j < 12; j += 1) {
                // Build RGB values
                ESP_ERROR_CHECK(strip->set_pixel(strip, j, 100 * detect_flag, 0.5 * i * 0, 0.5 * i * (1 - detect_flag)));
                ESP_ERROR_CHECK(strip->refresh(strip, 100));
            }
            vTaskDelay(pdMS_TO_TICKS(EXAMPLE_CHASE_SPEED_MS));
        }
        vTaskDelay(pdMS_TO_TICKS(EXAMPLE_CHASE_SPEED_MS));
    }
}
#elif defined CONFIG_ESP_LYRAT_MINI_V1_1_BOARD || defined CONFIG_ESP32_KORVO_V1_1_BOARD
#include "ws2812.h"
void led_init(void)
{
#if defined CONFIG_ESP_LYRAT_MINI_V1_1_BOARD
    gpio_config_t io_conf;
    io_conf.intr_type = (gpio_int_type_t) GPIO_PIN_INTR_DISABLE;
    io_conf.mode = GPIO_MODE_OUTPUT;
    io_conf.pull_up_en = (gpio_pullup_t) 1;

    uint64_t test = ((uint64_t)1 << 22);
    io_conf.pin_bit_mask = test;
    gpio_config(&io_conf);
    gpio_set_level(22, false);
#else
    init_ws2812();
#endif
}

void led_on(int gpio)
{
    gpio_set_level(gpio, true);
}

void led_off(int gpio)
{
    gpio_set_level(gpio, false);
}

void led_Task(void * arg)
{
    int on = 0;
    while (1) {
        if (detect_flag && on == 0) {
#ifdef CONFIG_ESP_LYRAT_MINI_V1_1_BOARD
            led_on(22);
#elif CONFIG_ESP32_KORVO_V1_1_BOARD
            blue_light_on();
#endif
            on = 1;
        } else if (detect_flag == 0 && on == 1) {
#ifdef CONFIG_ESP_LYRAT_MINI_V1_1_BOARD
            led_off(22);
#elif CONFIG_ESP32_KORVO_V1_1_BOARD
            light_off();
#endif
            on = 0;
        } else {
            vTaskDelay(10 / portTICK_RATE_MS);
        }
    }
}
#endif

#if defined CONFIG_ESP32_S3_BOX_BOARD || defined CONFIG_ESP32_S3_KORVO2_V3_BOARD
esp_err_t iot_dac_audio_play_8K(const uint16_t* data, int length, TickType_t ticks_to_wait)
{
    size_t bytes_write = 0;
    uint16_t *data_out = malloc(length * 8);
    for (int i = 0; i < length / 2; i++) {
        data_out[8 * i] = data[i];
        data_out[8 * i + 1] = data[i];
        data_out[8 * i + 2] = data[i];
        data_out[8 * i + 3] = data[i];
        data_out[8 * i + 4] = data[i];
        data_out[8 * i + 5] = data[i];
        data_out[8 * i + 6] = data[i];
        data_out[8 * i + 7] = data[i];
    }

    i2s_write(I2S_NUM_1, (const char*) data_out, length * 8, &bytes_write, ticks_to_wait);

    i2s_zero_dma_buffer(I2S_NUM_1);
    free(data_out);
    return ESP_OK;
}
#else
esp_err_t iot_dac_audio_play_8K(const uint16_t* data, int length, TickType_t ticks_to_wait)
{
    size_t bytes_write = 0;
    i2s_write(I2S_NUM_0, (const char*) data, length, &bytes_write, ticks_to_wait);
    i2s_zero_dma_buffer(I2S_NUM_0);

    return ESP_OK;
}
#endif
dac_audio_item_t playlist[] = {
    // {"ie_kaiji.h", ie_kaiji, sizeof(ie_kaiji)},
    {"wake_up_prompt_tone.h", wake_up_prompt_tone, sizeof(wake_up_prompt_tone)},
    {"m_1.h", m_1, sizeof(m_1)},
    {"m_2.h", m_2, sizeof(m_2)},
    {"m_3.h", m_3, sizeof(m_3)},
    {"m_4.h", m_4, sizeof(m_4)},
    {"m_5.h", m_5, sizeof(m_5)},
    {"m_6.h", m_6, sizeof(m_6)},
    {"m_7.h", m_7, sizeof(m_7)},
    {"m_8.h", m_8, sizeof(m_8)},
    {"m_9.h", m_9, sizeof(m_9)},
    {"m_10.h", m_10, sizeof(m_10)},
    {"m_11.h", m_11, sizeof(m_11)},
    {"m_12.h", m_12, sizeof(m_12)},
    {"m_13.h", m_13, sizeof(m_13)},
    {"m_14.h", m_14, sizeof(m_14)},
    {"m_15.h", m_15, sizeof(m_15)},
    {"m_16.h", m_16, sizeof(m_16)},
    {"m_17.h", m_17, sizeof(m_17)},
};

void wake_up_action(void)
{
    iot_dac_audio_play_8K(playlist[0].data, playlist[0].length, portMAX_DELAY);
}

void speech_commands_action(int command_id)
{
    iot_dac_audio_play_8K(playlist[command_id + 1].data, playlist[command_id + 1].length, portMAX_DELAY);
}