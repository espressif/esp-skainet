#include <string.h>
#include "bsp_board.h"
#include "driver/i2s_std.h"
#include "driver/gpio.h"
#include "esp_err.h"
#include "esp_log.h"

static const char *TAG = "custom_board";

static i2s_chan_handle_t rx_handle = NULL;
static i2s_chan_handle_t tx_handle = NULL;

static esp_err_t bsp_i2s_rx_init(uint32_t sample_rate)
{
    i2s_chan_config_t chan_cfg = I2S_CHANNEL_DEFAULT_CONFIG(I2S_NUM_1, I2S_ROLE_MASTER);
    esp_err_t ret = i2s_new_channel(&chan_cfg, NULL, &rx_handle);

    i2s_std_config_t std_cfg = {
        .clk_cfg = I2S_STD_CLK_DEFAULT_CONFIG(sample_rate),
        .slot_cfg = I2S_STD_PHILIPS_SLOT_DEFAULT_CONFIG(I2S_DATA_BIT_WIDTH_16BIT, I2S_SLOT_MODE_MONO),
        .gpio_cfg = {
            .mclk = I2S_GPIO_UNUSED,
            .bclk = GPIO_I2S_SCLK,
            .ws   = GPIO_I2S_LRCK,
            .dout = I2S_GPIO_UNUSED,
            .din  = GPIO_I2S_SDIN,
            .invert_flags = { .mclk_inv = false, .bclk_inv = false, .ws_inv = false },
        },
    };
    std_cfg.slot_cfg.slot_mask = I2S_STD_SLOT_LEFT;   // LR pin grounded = left slot

    ret |= i2s_channel_init_std_mode(rx_handle, &std_cfg);
    ret |= i2s_channel_enable(rx_handle);
    return ret;
}

static esp_err_t bsp_i2s_tx_init(uint32_t sample_rate, int channel_format, int bits_per_chan)
{
    i2s_chan_config_t chan_cfg = I2S_CHANNEL_DEFAULT_CONFIG(I2S_NUM_0, I2S_ROLE_MASTER);
    chan_cfg.auto_clear = true;
    esp_err_t ret = i2s_new_channel(&chan_cfg, &tx_handle, NULL);

    i2s_slot_mode_t slot_mode = (channel_format == 1) ? I2S_SLOT_MODE_MONO : I2S_SLOT_MODE_STEREO;
    i2s_data_bit_width_t bits = (bits_per_chan == 32) ? I2S_DATA_BIT_WIDTH_32BIT : I2S_DATA_BIT_WIDTH_16BIT;

    i2s_std_config_t std_cfg = {
        .clk_cfg = I2S_STD_CLK_DEFAULT_CONFIG(sample_rate),
        .slot_cfg = I2S_STD_PHILIPS_SLOT_DEFAULT_CONFIG(bits, slot_mode),
        .gpio_cfg = {
            .mclk = I2S_GPIO_UNUSED,
            .bclk = GPIO_I2S0_SCLK,
            .ws   = GPIO_I2S0_LRCK,
            .dout = GPIO_I2S0_DOUT,
            .din  = I2S_GPIO_UNUSED,
            .invert_flags = { .mclk_inv = false, .bclk_inv = false, .ws_inv = false },
        },
    };

    ret |= i2s_channel_init_std_mode(tx_handle, &std_cfg);
    ret |= i2s_channel_enable(tx_handle);
    return ret;
}

esp_err_t bsp_board_init(uint32_t sample_rate, int channel_format, int bits_per_chan)
{
    esp_err_t ret = ESP_OK;
    ret |= bsp_i2s_rx_init(16000);   // mic fixed at 16kHz — what AFE/MultiNet expect
    ret |= bsp_i2s_tx_init(sample_rate, channel_format, bits_per_chan);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "I2S init failed");
    }
    return ret;
}

esp_err_t bsp_get_feed_data(bool is_get_raw_channel, int16_t *buffer, int buffer_len)
{
    size_t bytes_read = 0;
    return i2s_channel_read(rx_handle, buffer, buffer_len, &bytes_read, portMAX_DELAY);
}

int bsp_get_feed_channel(void)
{
    return 1;   // single mono mic — not 4, that was Korvo's mic-array count
}

char* bsp_get_input_format(void)
{
    return "M";   // one plain mic channel, no AEC reference channel
}

esp_err_t bsp_audio_play(const int16_t* data, int length, TickType_t ticks_to_wait)
{
    size_t bytes_written = 0;
    return i2s_channel_write(tx_handle, data, length, &bytes_written, ticks_to_wait);
}

esp_err_t bsp_audio_set_play_vol(int volume) { return ESP_OK; }   // MAX98357A has no software volume control
esp_err_t bsp_audio_get_play_vol(int *volume) { if (volume) *volume = 100; return ESP_OK; }
esp_err_t bsp_sdcard_init(char *mount_point, size_t max_files) { return ESP_ERR_NOT_SUPPORTED; }
esp_err_t bsp_sdcard_deinit(char *mount_point) { return ESP_ERR_NOT_SUPPORTED; }
