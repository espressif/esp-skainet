/*
   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/
#include <stdio.h>
#include <stdlib.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_wn_iface.h"
#include "esp_wn_models.h"
#include "model_path.h"
#include "string.h"
#include "hiesp.h"
#include "hilexin.h"

void app_main(void *arg)
{
    srmodel_list_t *models = esp_srmodel_init("model");
    char *model_name = esp_srmodel_filter(models, ESP_WN_PREFIX, "hilexin");
    esp_wn_iface_t *wakenet = (esp_wn_iface_t*)esp_wn_handle_from_name(model_name);
    model_iface_data_t *model_data = wakenet->create(model_name, DET_MODE_95);

    int audio_chunksize = wakenet->get_samp_chunksize(model_data) * sizeof(int16_t);
    int16_t *buffer = (int16_t *) malloc(audio_chunksize);
    unsigned char* data = NULL;
    size_t data_size = 0;
    int chunks = 0;
    if (strstr(model_name, "hiesp") != NULL) {
        data = (unsigned char*)hiesp;
        data_size = sizeof(hiesp);
        printf("wake word: %s, size:%d\n", "hiesp",  data_size);
    } else if(strstr(model_name, "hilexin") != NULL) {
        data = (unsigned char*)hilexin;
        data_size = sizeof(hilexin);
        printf("wake word: %s, size:%d\n", "hilexin",  data_size);
    }

    while (1) {
        if ((chunks + 1)*audio_chunksize <= data_size) {
            memcpy(buffer, data + chunks * audio_chunksize, audio_chunksize);
        } else {
            break;
        }
        
        wakenet_state_t state = wakenet->detect(model_data, buffer);
        if (state == WAKENET_DETECTED) {
            printf("Detected\n");
        }
        chunks ++;
    }

    wakenet->destroy(model_data);
    vTaskDelete(NULL);
}

