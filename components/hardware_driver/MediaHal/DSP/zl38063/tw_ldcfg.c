#include <stdio.h>
#include <stdlib.h>  /* malloc, free, rand */

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

#undef SAVE_CFG_TO_FLASH   /*define this macro to save the cfg from RAM to flash*/

uint16 numElements;

dataArr* pCr2Buf;
/* fseekNunlines() -- The firmware file is an ascii text file.
 * the information from fseek will not be usefull
 * this is our own fseek equivalent
 */
static unsigned long fseekNunlines(FILE* BOOT_FD)
{
    uint32 line_count = 0;
    int c;

    while ( (c = fgetc(BOOT_FD)) != EOF ) {
        if ( c == '\n' )
            line_count++;
    }
    return line_count;
}

/* readCfgFile() use this function to
 * Read the Voice processing cr2 config file into RAM
 * filepath -- pointer to the location where to find the file
 * pCr2Buf -- the actual firmware data array will be pointed to this buffer
 */
static int readCfgFile(char* filepath)
{
    unsigned int reg[2], val[2], len;
    uint8 done = 0;
    uint16 index = 0;
    FILE* BOOT_FD;
    char* s;
    char line[512] = "";


    BOOT_FD = fopen(filepath, "rb");
    if (BOOT_FD != NULL) {
        len = fseekNunlines(BOOT_FD);
        if (len <= 0) {
            printf("Error: file is not of the correct format...\n");
            return -1;
        }
        printf("fileLength = %u\n", len);
        /*start at the beginning of the file*/
        //fseek(BOOT_FD, 0, SEEK_SET);

        /* allocate memory to contain the reg and val:*/
        pCr2Buf = (dataArr*) malloc(len * sizeof(dataArr));
        if (pCr2Buf == NULL) {
            printf ("not enough memory to allocate %u bytes.. ", len * sizeof(dataArr));
            return -1;
        }

        rewind(BOOT_FD);
        /*read and format the data accordingly*/
        numElements  = 0;
        do {
            s = fgets(line, 512, BOOT_FD);
            if (line[0] == ';') {
                continue;
            } else if (s != NULL) {
                numElements++;
                sscanf(line, "%x %c %x", reg, s, val);
                pCr2Buf[index].reg = reg[0];
                pCr2Buf[index].value = val[0];
                // printf("pCr2Buf[%d].reg pCr2Buf[%d].value  = 0x%04x\t0x%04x\n", index, index, pCr2Buf[index].reg, pCr2Buf[index].value);
                index++;
            } else { done = 1;}

        } while (done == 0);

        fclose(BOOT_FD);
        printf ("size of pCr2Buf = %u bytes.. \n", sizeof(pCr2Buf));
    } else {
        printf("Error: can't open file\n");
    }
    return 0;
}

/*This example host app load the *.s3 firmware to the device RAM. Optionally save it to flash
 * Then start the firmware from the execution address in RAM
 * It then stops the firmware - Load the cr2 file into RAM. Optionally save it to flash
 * Then resstarts the firmware
 */

int main (int argc, char** argv)
{

    VprocStatusType status = VPROC_STATUS_SUCCESS;


    if (argc != 2) {
        printf("Error: argc = %d - missing %d arg(s)... \n", argc, 3 - (argc - 1));
        printf("command Usage:%s ConfigPath\n", argv[0]);
        exit(1);
    }
    printf(":%s %s %s\n", argv[0], argv[1], argv[2]);


    /*global file handle*/
    status = VprocTwolfHbiInit();

    if (status < 0) {
        perror("tw_spi_access open");
        return -1;
    }

    if (readCfgFile(argv[1]) < 0) {
        printf("Error:read %s file\n", argv[1]);
    }
    printf("a- Reading config file to host RAM - done....\n");


    printf("c- Loading the config file into the device RAM\n");
    status  = VprocTwolfLoadConfig(pCr2Buf, numElements);
    if (status != VPROC_STATUS_SUCCESS) {
        printf("Error %d:VprocTwolfLoadConfig()\n", status);
        VprocTwolfHbiCleanup();
        return -1;
    }

#ifdef SAVE_CONFIG_TO_FLASH
    status = VprocTwolfSaveCfgToFlash();
    if (status != VPROC_STATUS_SUCCESS) {
        printf("Error %d:VprocTwolfSaveCfgToFlash()\n", status);
        VprocTwolfHbiCleanup();
        return -1;
    }
    printf("d- Saving config to flash- done....\n");
#endif

    printf("e- Loading config record - done....\n");
    free(pCr2Buf);
    pCr2Buf = NULL;
    VprocTwolfHbiCleanup();

    return 0;
}


