#include "vproc_common.h"
#include "vprocTwolf_access.h"

/*NOTE: notice that the *.c code are included in the apps-
* This is because the compiler I'm using requires that
* But if your makefile is such that compiler knows where to find these files
* then remove the #include *.c below
*/
#if 1
#include "vproc_common.c"
#include "vprocTwolf_access.c"
#endif

#undef SAVE_IMAGE_TO_FLASH /*define this macro to save the firmware from RAM to flash*/

/*quick test*/

/*This example host app load the *.s3 firmware to the device RAM. Optionally save it to flash
 * Then start the firmware from the execution address in RAM
 */
int main (int argc, char** argv)
{

    VprocStatusType status = VPROC_STATUS_SUCCESS;
    FILE* BOOT_FD;
    char line[256] = "";


    if (argc != 2) {
        printf("Error: argc = %d - missing %d arg(s)... \n", argc, 3 - (argc - 1));
        printf("command Usage:%s firmwarePath\n", argv[0]);
        exit(1);
    }
    printf(":%s %s %s\n", argv[0], argv[1], argv[2]);


    BOOT_FD = fopen(argv[1], "rb");
    if (BOOT_FD == NULL) {
        printf("Error: can't open file %s\n", argv[1]);
        return -1;
    }

    /*global file handle*/
    status = VprocTwolfHbiInit();
    if (status < 0) {
        perror("tw_spi_access open");
        return -1;
    }

    printf("1- Opening firmware file - done....\n");

    status  = VprocTwolfHbiBootPrepare();
    if (status != VPROC_STATUS_SUCCESS) {
        printf("Error %d:VprocTwolfHbiBootPrepare()\n", status);
        fclose(BOOT_FD);
        VprocHALcleanup();
        return -1;
    }
    printf("-- Boot prepare - done....\n");

    while (fgets(line, 256, BOOT_FD) != NULL) {
        status = VprocTwolfHbiBootMoreData(line);
        if (status == VPROC_STATUS_BOOT_LOADING_MORE_DATA) {
            continue;
        } else if (status == VPROC_STATUS_BOOT_LOADING_CMP) {

            break ;
        } else if (status != VPROC_STATUS_SUCCESS) {
            printf("Error %d:VprocTwolfHbiBootMoreData()\n", status);
            fclose(BOOT_FD);
            VprocHALcleanup();
            return -1;
        }
    }
    printf("-- Firmware data transfer - done....\n");
    fclose(BOOT_FD);
    /*clean up and verify that the boodloading completed correctly*/
    status  = VprocTwolfHbiBootConclude();
    if (status != VPROC_STATUS_SUCCESS) {
        printf("Error %d:VprocTwolfHbiBootConclude()\n", status);
        VprocHALcleanup();
        return -1;
    }

    printf("2- Loading firmware - done....\n");
#ifdef SAVE_IMAGE_TO_FLASH
    printf("-- Saving firmware to flash....\n");
    status = VprocTwolfSaveImgToFlash();
    if (status != VPROC_STATUS_SUCCESS) {
        printf("Error %d:VprocTwolfSaveImgToFlash()\n", status);
        VprocHALcleanup();
        return -1;
    }
    printf("-- Saving firmware to flash....done\n");

#endif

    status  = VprocTwolfFirmwareStart();
    if (status != VPROC_STATUS_SUCCESS) {
        printf("Error %d:VprocTwolfFirmwareStart()\n", status);
        VprocHALcleanup();
        return -1;
    }

    printf("Device boot loading completed successfully...\n");

    VprocHALcleanup();

    return 0;
}


