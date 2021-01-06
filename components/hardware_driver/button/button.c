#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <string.h>
#include "driver/gpio.h"
#include "driver/adc.h"
#ifndef CONFIG_IDF_TARGET_ESP32S2
#include "esp_adc_cal.h"
#endif
#include "button.h"

#define NO_OF_SAMPLES   1
#define DEFAULT_VREF    1100

#ifndef CONFIG_IDF_TARGET_ESP32S2
static esp_adc_cal_characteristics_t *adc_chars;
static const adc_unit_t unit = ADC_UNIT_1;
#endif
static const adc_channel_t channel = ADC_CHANNEL_3;
static const adc_atten_t atten = ADC_ATTEN_11db;


static uint32_t _button_voltage;

void buttondetTask(void *arg)
{
    while (1) {
        uint32_t adc_reading = 0;
        for (int i = 0; i < NO_OF_SAMPLES; i++) {
            adc_reading += adc1_get_raw((adc1_channel_t)channel);
        }
        adc_reading /= NO_OF_SAMPLES;
#ifdef CONFIG_IDF_TARGET_ESP32
        _button_voltage = esp_adc_cal_raw_to_voltage(adc_reading, adc_chars);
#else
        _button_voltage = (float)adc_reading * 3300 / 4096;
#endif
        vTaskDelay(100 / portTICK_PERIOD_MS);
    }
}

void button_init()
{
    adc1_config_width(ADC_WIDTH_12Bit);
    adc1_config_channel_atten(channel, atten);
#ifndef CONFIG_IDF_TARGET_ESP32S2
    //Characterize ADC
    adc_chars = calloc(1, sizeof(esp_adc_cal_characteristics_t));
    esp_adc_cal_characterize(unit, atten, ADC_WIDTH_12Bit, DEFAULT_VREF, adc_chars);
#endif

    xTaskCreatePinnedToCore(&buttondetTask, "buttondet", 2 * 1024, NULL, 8, NULL, 1);
}

void button_detect(char *s)
{
    uint32_t voltage1 = _button_voltage;
#ifdef CONFIG_ESP32_KORVO_V1_1_BOARD
    if (voltage1 > 1970 && voltage1 < 2020)
    {
    	sprintf(s, "mode");
    }
    else if (voltage1 > 1640 && voltage1 < 1690)
    {
    	sprintf(s, "play");
    }
    else if (voltage1 > 1100 && voltage1 < 1150)
    {
    	sprintf(s, "set");
    }
    else if (voltage1 > 370 && voltage1 < 420)
    {
    	sprintf(s, "vol+");
    }
    else if (voltage1 > 800 && voltage1 < 850)
    {
    	sprintf(s, "vol-");
    }
    else if (voltage1 > 2400 && voltage1 < 2450)
    {
    	sprintf(s, "rec");    	
    }
    else
    {
    	sprintf(s, "null");
    }
#elif CONFIG_ESP_LYRAT_MINI_V1_1_BOARD
    if (voltage1 > 2300 && voltage1 < 2500)
    {
    	sprintf(s, "rec");
    }
    else if (voltage1 > 1900 && voltage1 < 2100)  //1970
    {
    	sprintf(s, "mode");
    }
    else if (voltage1 > 1500 && voltage1 < 1700) //1566
    {
    	sprintf(s, "play");
    }
    else if (voltage1 > 1100 && voltage1 < 1300)  //1170
    {
    	sprintf(s, "set");
    }
    else if (voltage1 > 700 && voltage1 < 900)  //795
    {
    	sprintf(s, "vol-");
    }
    else if (voltage1 > 300 && voltage1 < 500) //370
    {
    	sprintf(s, "vol+");    	
    }
    else
    {
    	sprintf(s, "null");
    }
#endif

}