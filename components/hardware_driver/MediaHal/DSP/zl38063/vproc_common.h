#ifndef VPROC_COMMON_H
#define VPROC_COMMON_H

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "esp_log.h"

#define DEBUG_LOGD ESP_LOGD
#define DEBUG_LOGE ESP_LOGE
#define DEBUG_LOGI ESP_LOGI
/*This header includes some platform dependent data types*/
#include "vproc_data_types.h"

//#define RETRY_COUNT 100
#define VPROC_TIMEOUT 500

#define TAG_SPI "SPI"


/* external defines */
#undef VPROC_DEBUG


/*create a 16-bit word out of two bytes*/
#define MAKE16(a, b)      (unsigned short)(((unsigned short)(b) << 8) | \
                      (unsigned short)(a))
/*create a 32-bit word out of 4 bytes*/

#define MAKE32(a, b, c, d)  (unsigned long)(((unsigned long)d << 24) | ((unsigned long)c << 16) | ((unsigned long)b << 8) | ((unsigned long)a))
/*
 * debug - print the function name and line number for the source of the error
 * the line number count start at 1 and not 0
 */


/*SPI specific settings*/



/*unsigned char deviceType;*/
/*device/access Status  codes*/
typedef enum VprocStatusType {
    VPROC_STATUS_SUCCESS      =      0,
    VPROC_STATUS_FAILURE,
    VPROC_STATUS_INIT_FAILED,
    VPROC_STATUS_WR_FAILED,
    VPROC_STATUS_RD_FAILED,
    VPROC_STATUS_FW_LOAD_FAILED,
    VPROC_STATUS_CFG_LOAD_FAILED,
    VPROC_STATUS_CLOSE_FAILED,
    VPROC_STATUS_FW_SAVE_FAILED,
    VPROC_STATUS_GFG_SAVE_FAILED,
    VPROC_STATUS_MAU_NOT_READY,
    VPROC_STATUS_CHK_FAILED,
    VPROC_STATUS_FUNC_NOT_SUPPORTED,
    VPROC_STATUS_INVALID_ARG,
    VPROC_STATUS_ERR_VTD_CODE,
    VPROC_STATUS_ERR_VERIFY,
    VPROC_STATUS_DEVICE_BUSY,
    VPROC_STATUS_ERR_HBI,
    VPROC_STATUS_ERR_IMAGE,
    VPROC_STATUS_MAILBOX_BUSY,
    VPROC_STATUS_CMDREG_BUSY,
    VPROC_STATUS_IN_CRTCL_SECTN,
    VPROC_STATUS_BOOT_LOADING_MORE_DATA,
    VPROC_STATUS_BOOT_LOADING_CMP,
    VPROC_STATUS_DEV_NOT_INITIALIZED,

} VprocStatusType;

/* Device Reset modes*/
typedef enum VprocResetMode {
    VPROC_RST_HARDWARE_ROM  =  0, /*hardware reset -reset the device and reload the firmware from flash*/
    VPROC_RST_HARDWARE_RAM  =  1, /*hardware reset -reset the device and reload the firmware from RAM*/
    VPROC_RST_SOFTWARE      =  2,
    VPROC_RST_AEC   =  3,              /*software reset -reset and runs the firmware from RAM*/
    VPROC_RST_BOOT = 4
} VprocResetMode;

typedef enum vProcDeviceType {
    VPROC_DEV_GALILEO       =  1, /*Galileo devices: ZL38004, ZL38012, ZL38005*/
    VPROC_DEV_TIMBERWOLF    =  2  /*Timberwolf: ZL38040*/
} VprocDeviceType;

extern void VprocHALcleanup(void);
extern int VprocHALInit(void);
extern void Vproc_msDelay(unsigned short time);
extern void VprocWait(unsigned long int time);
extern int VprocHALWrite(unsigned short val);
extern int VprocHALRead(unsigned short* pVal);
#endif /* VPROC_COMMON_H */
