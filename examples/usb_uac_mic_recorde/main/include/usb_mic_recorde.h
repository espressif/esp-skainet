#pragma once

#ifndef __USB_MIC_RECORDE_H
#define __USB_MIC_RECORDE_H

#ifdef __cplusplus
extern "C" {
#endif

#include "esp_err.h"
#include "ringbuf.h"

/**
 * @brief
 *
 * @return ringbuf_handle_t
 */
ringbuf_handle_t mic_recorde_init();


#ifdef __cplusplus
}
#endif

#endif // __USB_MIC_RECORDE_H