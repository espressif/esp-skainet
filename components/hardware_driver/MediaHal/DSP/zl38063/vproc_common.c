#include "vproc_common.h"
#include <stdio.h>
#include "driver/spi_master.h"
#include "soc/gpio_struct.h"
#include "driver/gpio.h"
#include "mbedtls/net.h"
#include "lwip/def.h"
#include "board.h"

static spi_device_handle_t g_spi = NULL;

int VprocHALInit(void)
{
    /*if the customer platform requires any init
    * then implement such init here.
    * Otherwise the implementation of this function is complete
    */
    esp_err_t ret = ESP_OK;

    spi_bus_config_t buscfg = {
        .miso_io_num = MICRO_SEMI_SPI_MISO,
        .mosi_io_num = MICRO_SEMI_SPI_MOSI,
        .sclk_io_num = MICRO_SEMI_SPI_CLK,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1
    };

    spi_device_interface_config_t devcfg = {
        .clock_speed_hz = 10000000,              // Clock out at 10 MHz
        .mode = 0,                              // SPI mode 0
        .spics_io_num = MICRO_SEMI_SPI_CS,       // CS pin
        .queue_size = 6,                        // We want to be able to queue 7 transactions at a time
    };
    //Initialize the SPI bus
    if (g_spi) {
        return ret;
    }
    ret = spi_bus_initialize(HSPI_HOST, &buscfg, 0);
    assert(ret == ESP_OK);
    ret = spi_bus_add_device(HSPI_HOST, &devcfg, &g_spi);
    assert(ret == ESP_OK);
    gpio_set_pull_mode(0, GPIO_FLOATING);

    return ret;
}

/*HAL clean up function - To close any open file connection
 * microsemi_spis_tw kernel char driver
 *
 * return: a positive integer value for success, a negative value for failure
 */
void VprocHALcleanup(void)
{
    /*if the customer platform requires any clean up function
    * then implement such function here.
    * Otherwise the implementation of this function is complete
    */
    int ret = 0;
    ret = spi_bus_remove_device(g_spi);
    assert(ret == ESP_OK);
    ret = spi_bus_free(HSPI_HOST);
    assert(ret == ESP_OK);
}
/*Note - These functions are PLATFORM SPECIFIC- They must be modified
 *       accordingly
 **********************************************************************/
/* Vproc_msDelay(): use this function to
 *     force a delay of specified time in resolution of milli-second
 *
 * Input Argument: time in unsigned 16-bit
 * Return: none
 */

void Vproc_msDelay(unsigned short time)
{
    //usleep(time * 1000); /*PLATFORM SPECIFIC - system wait in ms*/
    ets_delay_us(time * 1000);
}

/* VprocWait(): use this function to
*     force a delay of specified time in resolution of 125 micro-Seconds
*
* Input Argument: time in unsigned 32-bit
* Return: none
*/
void VprocWait(unsigned long int time)
{
    //usleep(125 * time); /*system wait in frame*/
    ets_delay_us(time * 1000);
}

#define BIGENDIAN 1

/* This is the platform dependant low level spi
 * function to write 16-bit data to the ZL380xx device
 */
int VprocHALWrite(unsigned short val)
{
    /*Note: Implement this as per your platform*/
    esp_err_t ret;
    spi_transaction_t t;
    unsigned short data = 0;
    //DEBUG_LOGE(TAG_SPI, "VprocHALWrite start")
    memset(&t, 0, sizeof(t));       //Zero out the transaction
    t.length = sizeof(unsigned short) * 8; //Len is in bytes, transaction length is in bits.
#if BIGENDIAN
    data = htons(val);
    t.tx_buffer = &data;             //Data
#else
    t.tx_buffer = &val;
#endif
    //t.user=(void*)1;                //D/C needs to be set to 1
    //printf("write:%d-%04x-%04x\n", t.length, data, val);
    //DEBUG_LOGE(TAG_SPI, "spi_device_transmit start")
    ret = spi_device_transmit(g_spi, &t); //Transmit!
    //DEBUG_LOGE(TAG_SPI, "spi_device_transmit end %d", ret)
    assert(ret == ESP_OK);          //Should have had no issues.

    return 0;
}

/* This is the platform dependant low level spi
 * function to read 16-bit data from the ZL380xx device
 */
int VprocHALRead(unsigned short* pVal)
{
    /*Note: Implement this as per your platform*/
    esp_err_t ret;
    spi_transaction_t t;
    unsigned short data = 0;

    memset(&t, 0, sizeof(t));           //Zero out the transaction
    t.length = sizeof(unsigned short) * 8;
    t.rxlength = sizeof(unsigned short) * 8; //Len is in bytes, transaction length is in bits.
    t.rx_buffer = &data;
    ret = spi_device_transmit(g_spi, &t);   //Transmit!
#if BIGENDIAN
    *pVal = ntohs(data);
#else
    *pVal = data;
#endif
    // printf("read:%d-%04x-%04x\n", t.rxlength, data, *pVal);
    assert(ret == ESP_OK);              //Should have had no issues.

    return 0;
}

