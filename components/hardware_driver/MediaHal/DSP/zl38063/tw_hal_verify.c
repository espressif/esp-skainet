#include <stdio.h>
#include <stdlib.h>  /* malloc, free, rand */

#include "vproc_common.h"
#include "zl38063_config.h"
#include "vprocTwolf_access.h"
#include "zl38063_firmware.h"

/*NOTE: notice that the *.c code are included in the apps-
* This is because the compiler I'm using requires that
* But if your makefile is such that compiler knows where to find these files
* then remove the #include *.c below
*/
#if 1
#include "vproc_common.c"
#include "vprocTwolf_access.c"
#include "zl38063_config.c"
#include "zl38063_firmware.c"
#endif

#undef SAVE_IMAGE_TO_FLASH /*define this macro to save the firmware from RAM to flash*/
#undef SAVE_CFG_TO_FLASH   /*define this macro to save the cfg from RAM to flash*/
/*quick test*/

#define TW_HAL_VERIFY_DEBUG

#define MAX_WORDS_FOR_MULTIWORD_ACCESS_TEST  125


/*LoadFwrConfig_Alt - to load a converted *s3, *cr2 to c code into the device.
* Basically instead of loading the *.s3, *cr2 directly,
* use the tw_convert tool to convert the ascii hex fwr mage into code and compile
* with the application
*
* input arg: mode:  0 - load both firmware and confing
*                   1 - load firmware only
*                   2 - load config only
*/
VprocStatusType LoadFwrConfig_Alt(uint8 mode)
{
    VprocStatusType status = VPROC_STATUS_SUCCESS;
    if ((mode == 0) || (mode == 1)) {
        twFirmware st_Firmware;
        st_Firmware.st_Fwr = (twFwr*)st_twFirmware;
        st_Firmware.twFirmwareStreamLen = (uint16)firmwareStreamLen;
        st_Firmware.execAddr  = (uint32)executionAddress;
        st_Firmware.havePrgmBase = (uint8)haveProgramBaseAddress;
        st_Firmware.prgmBase = (uint32)programBaseAddress;

        printf("\t1- Firmware boot loading started ....\n");

        status  = VprocTwolfHbiBoot_alt(&st_Firmware);
        if (status != VPROC_STATUS_SUCCESS) {
            printf("Error %d:VprocTwolfHbiBoot()\n", status);

            return -1;
        }

        printf("\t2- Loading the image to RAM....done\n");
#ifdef SAVE_IMAGE_TO_FLASH
        printf("\t-- Saving firmware to flash....\n");
        status = VprocTwolfSaveImgToFlash();
        if (status != VPROC_STATUS_SUCCESS) {
            printf("Error %d:VprocTwolfSaveImgToFlash()\n", status);

            return status;
        }
        printf("\t-- Saving firmware to flash....done\n");

#endif

        status  = VprocTwolfFirmwareStart();
        if (status != VPROC_STATUS_SUCCESS) {
            printf("Error %d:VprocTwolfFirmwareStart()\n", status);

            return status;
        }
    }
    if ((mode == 0) || (mode == 2)) {
        printf("\t3- Loading the config file into the device RAM....\n");

        status  = VprocTwolfLoadConfig((dataArr*)st_twConfig,
                                       (uint16)configStreamLen);
        if (status != VPROC_STATUS_SUCCESS) {
            printf("Error %d:VprocTwolfLoadConfig()\n", status);

            return status;
        }
#ifdef SAVE_CFG_TO_FLASH
        printf("\t-- Saving config to flash....\n");
        status = VprocTwolfSaveCfgToFlash();
        if (status != VPROC_STATUS_SUCCESS) {
            printf("Error %d:VprocTwolfSaveCfgToFlash()\n", status);

            return status;
        }
        printf("\t-- Saving config to flash....done\n");

#endif
    }
    {  /*Verify that the boot loading PASS or Fail*/
        uint16 val = 0;

        status  = VprocTwolfHbiRead(0x0022, 1, &val);
        if (status != VPROC_STATUS_SUCCESS) {
            printf("Error %d:VprocTwolfHbiRead()\n", status);
            VprocTwolfHbiCleanup();
            return -1;
        }
        if ((val == 38040) ||
            (val == 38050) ||
            (val == 38060) ||
            (val == 38080) ||
            (val == 38051) ||
            (val == 38041))
            printf("\tDevice boot loading completed successfully...\n");
        else {
            printf("\tDevice boot loading failed!!!...\n");
            return VPROC_STATUS_FAILURE;
        }
    }

    /*Firmware reset - in order for the configuration to take effect
     * NOTE:  The ZL38040 needs a soft reset for the uploaded configuration
     *       to take effect. This soft-reset is sent below
     *       if the ZL38040 is an I2S slave, if the I2S master is not stable
     *       at the time of this reset, then that reset will not take effect.
     *       In that case the host has to to simply resend the reset
     *       command once the I2S master
     *       is up and running and is at a stable state.
     */
    status  = VprocTwolfReset(VPROC_RST_SOFTWARE);
    if (status != VPROC_STATUS_SUCCESS) {
        printf("Error %d:VprocTwolfReset()\n", status);

        return status;
    }

    printf("\tDevice boot loading completed successfully...\n");
    return status;
}


int test_zl38063(void* arg)
{
    int status = 0;
    uint16  cmdword = 0;
    uint16 val[MAX_WORDS_FOR_MULTIWORD_ACCESS_TEST];
    uint8 numwords = 0;
    uint16 tempbuf[MAX_WORDS_FOR_MULTIWORD_ACCESS_TEST];
    uint16 i = 0;
#ifdef TW_HAL_VERIFY_DEBUG
    uint16 j = 0;
#endif
    status = VprocTwolfHbiInit();
    if (status < 0) {
        perror("tw_spi_access open");
        return -1;
    }
    if ((MAX_WORDS_FOR_MULTIWORD_ACCESS_TEST > 125) ||
        (MAX_WORDS_FOR_MULTIWORD_ACCESS_TEST < 2)) {

        printf("MAX_WORDS_FOR_MULTIWORD_ACCESS_TEST must be within 2 and 126\n");
    }
    memset(val, 0, MAX_WORDS_FOR_MULTIWORD_ACCESS_TEST);
    memset(tempbuf, 0, MAX_WORDS_FOR_MULTIWORD_ACCESS_TEST);

    printf("Test 1 - Verifying that the device is present and working ....\n");
    cmdword = 0x00C;
    numwords = 2;
    val[0] = 0x1234;
    val[1] = 0x5678;
    status  = VprocTwolfHbiWrite(cmdword, numwords, val);
    if (status != VPROC_STATUS_SUCCESS) {
        printf("Error %d:VprocTwolfHbiWrite()\n", status);
        VprocHALcleanup();
        return -1;
    }
#ifdef TW_HAL_VERIFY_DEBUG
    j = 0;
    for (i = 0; i < numwords; i++) {
        printf("\twr: addr 0x%04x = 0x%04x\n", (cmdword + j), val[i]);
        j = j + 2;
    }
#endif
    status  = VprocTwolfHbiRead(cmdword, numwords, tempbuf);
    if (status != VPROC_STATUS_SUCCESS) {
        printf("Error %d:VprocTwolfHbiRead()\n", status);
        VprocTwolfHbiCleanup();
        return -1;
    }
#ifdef TW_HAL_VERIFY_DEBUG
    j = 0;
    for (i = 0; i < numwords; i++) {
        printf("\tRD: addr 0x%04x = 0x%04x\n", (cmdword + j), tempbuf[i]);
        j = j + 2;
    }
#endif
    if ((tempbuf[0] != 0x1234) && (tempbuf[1] != 0x5600)) {
        printf("Test 1 - completed - FAIL!!!\n\n");
        return -1;
    }


    printf("Test 1 - completed - PASS\n\n");

    status  = VprocTwolfReset(0);
    if (status != VPROC_STATUS_SUCCESS) {
        printf("Error %d:VprocTwolfHbiRead()\n", status);
        VprocTwolfHbiCleanup();
        return -1;
    }
    printf("Device reset completed successfully...\n");


    printf("Test 2 - Verifying single word write/read access ....\n");
    cmdword = 0x0300;
    val[0] = 0x4008;
    numwords = 1;
    status  = VprocTwolfHbiWrite(cmdword, numwords, val);
    if (status != VPROC_STATUS_SUCCESS) {
        printf("Error %d:VprocTwolfHbiWrite()\n", status);
        VprocTwolfHbiCleanup();
        return -1;
    }
#ifdef TW_HAL_VERIFY_DEBUG
    j = 0;
    for (i = 0; i < numwords; i++) {
        printf("\twr: addr 0x%04x = 0x%04x\n", (cmdword + j), val[i]);
        j = j + 2;
    }
#endif
    status  = VprocTwolfHbiRead(cmdword, numwords, val);
    if (status != VPROC_STATUS_SUCCESS) {
        printf("Error %d:VprocTwolfHbiRead()\n", status);
        VprocTwolfHbiCleanup();
        return -1;
    }
#ifdef TW_HAL_VERIFY_DEBUG
    j = 0;
    for (i = 0; i < numwords; i++) {
        printf("\tRD: addr 0x%04x = 0x%04x\n", (cmdword + j), val[i]);
        j = j + 2;
    }
#endif
    if ((val[0] != 0x4008)) {
        printf("Test 2 - completed - FAIL!!!\n\n");
        return -1;
    }
    printf("Test 2 - completed - PASS\n\n");


    printf("Test 3 - Verifying multiple words write/read access ....\n");

    /* Fill the data buffer with unique data values. */
    for ( i = 0 ; i < MAX_WORDS_FOR_MULTIWORD_ACCESS_TEST ; i++ ) {
        val[i] = i | ((0xFF - i) << 8);
    }

    cmdword = 0x0300;

    numwords = MAX_WORDS_FOR_MULTIWORD_ACCESS_TEST;
    status  = VprocTwolfHbiWrite(cmdword, numwords, val);
    if (status != VPROC_STATUS_SUCCESS) {
        printf("Error %d:VprocTwolfHbiWrite()\n", status);
        VprocTwolfHbiCleanup();
        return -1;
    }
#ifdef TW_HAL_VERIFY_DEBUG
    j = 0;
    for (i = 0; i < numwords; i++) {
        printf("\twr: addr 0x%04x = 0x%04x\n", (cmdword + j), val[i]);
        j = j + 2;
    }
#endif
    status  = VprocTwolfHbiRead(cmdword, numwords, tempbuf);
    if (status != VPROC_STATUS_SUCCESS) {
        printf("Error %d:VprocTwolfHbiRead()\n", status);
        VprocTwolfHbiCleanup();
        return -1;
    }
#ifdef TW_HAL_VERIFY_DEBUG
    j = 0;
    for (i = 0; i < numwords; i++) {
        printf("\tRD: addr 0x%04x = 0x%04x =? 0x%04x\n", (cmdword + j), tempbuf[i], val[i]);
        j = j + 2;
    }
#endif
    j = 0;
    for ( i = 0 ; i < MAX_WORDS_FOR_MULTIWORD_ACCESS_TEST ; i++ ) {
        if (tempbuf[i] != val[i]) {
            printf("\tRD: addr 0x%04x = 0x%04x =? 0x%04x\n", (cmdword + j), tempbuf[i], val[i]);
            printf("Test 3 - completed - FAIL!!!\n\n");
            return -1;
        }
        j = j + 2;
    }
    printf("Test 3 - completed - PASS\n\n");


    printf("Test 4 - Verifying the firmware/config boot loading ....\n");
    if (LoadFwrConfig_Alt(0) !=  VPROC_STATUS_SUCCESS) {
        printf("Device boot loading failed.....\n");
        printf("Test 4 - completed - FAIL!!!\n\n");

    } else
        printf("Test 4 - completed - PASS\n\n");

    printf("\n");
    VprocTwolfHbiCleanup();


    return 0;
}

