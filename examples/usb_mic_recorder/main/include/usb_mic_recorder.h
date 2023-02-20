/* SPDX-FileCopyrightText: 2022-2023 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#pragma once

#ifndef __USB_MIC_RECORDER_H
#define __USB_MIC_RECORDER_H

#ifdef __cplusplus
extern "C" {
#endif

#include "esp_err.h"
#include "ringbuf.h"

/**
 * @brief  Initializing the USB MIC function
 *
 * @return ringbuf_handle_t
 */
ringbuf_handle_t mic_recorder_init();


#ifdef __cplusplus
}
#endif

#endif // __USB_MIC_RECORDEr_H