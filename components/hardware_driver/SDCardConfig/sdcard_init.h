#ifndef _GET_SDCARD_H_
#define _GET_SDCARD_H_

esp_err_t sd_card_mount(const char* basePath);

int FatfsComboWrite(const void* buffer, int size, int count, FILE* stream);

#endif
