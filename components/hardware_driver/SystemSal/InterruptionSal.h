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

#ifndef _INTERRUPTION_SAL_H_
#define _INTERRUPTION_SAL_H_

#include "esp_intr_alloc.h"

#ifdef __cplusplus
extern "C" {
#endif

/*
    Please refer to "esp-idf\components\soc\esp32\include\soc\soc.h" to learn the distribution of all the interruption.
*/

#define GPIO_INTER_FLAG     ESP_INTR_FLAG_LEVEL1
#define TOUCH_INTER_FLAG    ESP_INTR_FLAG_LEVEL2 //can not be set as 1
#define I2S_INTER_FLAG      ESP_INTR_FLAG_LEVEL2

int GpioInterInstall();


#ifdef __cplusplus
}
#endif

#endif  //_INTERRUPTION_SAL_H_
