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

#ifndef _IM501_SPI_COMMEN_H_
#define _IM501_SPI_COMMEN_H_

#define  iM501_I2C_REG                   0x10
#define  iM501_SPI_REG                   0x01
#define  iM501_I2C_SPI_REG               0x12//bit2 = 1 means I2C mode, default is SPI mode

#define  TO_DSP_CMD_ADDR                 (0x0FFFBFF8)
#define  TO_DSP_CMD_OFFSET_ATTR( x )     ( (x) & 0xFFFF )
#define  TO_DSP_CMD_OFFSET_STAT( x )     ( ((x) & 0xFF) << 24 )
#define  TO_DSP_CMD_REQ_START_BUF_TRANS   0x19
#define  TO_DSP_CMD_REQ_STOP_BUF_TRANS    0x1D
#define  TO_DSP_CMD_REQ_ENTER_PSM         0x0D

#define  TO_HOST_CMD_KEYWORD_DET          0x40
#define  TO_HOST_CMD_DATA_BUF_RDY         0x41

#define  TO_DSP_FRAMECOUNTER_ADDR		0x0FFFBEFC

#define  TO_HOST_CMD_ADDR                (0x0FFFBFFC)
#define  HW_VOICE_BUF_START              (0x0FFF3EE0)  // DRAM voice buffer : 0x0FFF3EE0 ~ 0x0FFFBEDF = 32kB
#define  HW_VOICE_BUF_BANK_SIZE          (1024*2)  //2kB
#define  HW_BUF_RX_L                     (0x0FFFE000)  //1kB
#define  HW_BUF_RX_R                     (0x0FFFE400)  //1kB
#define  HW_BUF_RX_SIZE                  (2048)
#define  TO_HOST_CMD_OFFSET_ATTR( x )     ( (x) & 0xFFFF )
#define  TO_HOST_CMD_OFFSET_CMD( x )      ( ((x) & 0xFF) << 16 )
#define  TO_HOST_CMD_OFFSET_STAT( x )     ( ((x) & 0xFF) << 24 )
#define  GET_HOST_CMD_OFFSET_ATTR( x )    ( (x) & 0xFFFF )
#define  GET_HOST_CMD_OFFSET_CMD( x )     ( ((x)>>16) & 0xFF )
#define  GET_HOST_CMD_OFFSET_STAT( x )    ( ((x)>>24) & 0xFF )

#define  CHECK_LAST_PACK( x )            ( ((x)>>15) & 0x01 )
#define  GET_PACK_LEN( x )               ( (x) & 0x7F )
#define  GET_PACK_INDEX( x )             ( (x) & 0x7F )

#define  CMD_STAT_INIT                   0xFF
#define  CMD_STAT_DONE                   0x00

#define  IM501_I2C_CMD_DM_WR             0x2B //dram 2-byte write command, For burst mode, only can be 2 bytes
#define  IM501_I2C_CMD_DM_RD             0x27 //dram rea command d, Normal W/R, can be 1,2,4 bytes.
#define  IM501_I2C_CMD_IM_WR             0x0D //iram 4-byte write command
#define  IM501_I2C_CMD_IM_RD             0x07 //iram read command
#define  IM501_I2C_CMD_REG_WR_1          0x48 //i2c register 1-byte write command
#define  IM501_I2C_CMD_REG_WR_2          0x4A //i2c register 2-byte write command
#define  IM501_I2C_CMD_REG_RD            0x46 //i2c register read command, only support one byte read
#define  IM501_I2C_CMD_DM_WR_BST         0xAB //4a?
#define  IM501_I2C_CMD_DM_RD_BST         0xA7
#define  IM501_I2C_CMD_IM_WR_BST         0x8B
#define  IM501_I2C_CMD_IM_RD_BST         0x87
#define  IM501_SPI_CMD_DM_WR             0x05
#define  IM501_SPI_CMD_DM_RD             0x01
#define  IM501_SPI_CMD_IM_WR             0x04
#define  IM501_SPI_CMD_IM_RD             0x00
#define  IM501_SPI_CMD_REG_WR            0x06
#define  IM501_SPI_CMD_REG_RD            0x02


#define  IM501_LEFT_CHANNEL_REG          (0x0FFFBF88)
#define  IM501_RIGHT_CHANNEL_REG         (0x0FFFBF8C)

enum channel {
    LEFT_CHANNEL,
    RIGHT_CHANNEL
};

enum source {
    MIC0,
    MIC1,
    LINE_OUT
};



#define SUCCESS                          0u
#define NO_ERR                           0u

#define SPI_BUS_ERR                      179u
#define I2C_BUS_ERR                      180u
#define TO_501_CMD_ERR                   181u

#define SPI_FIFO_SIZE                    (3072)
#define SPI_BUF_SIZE                     (1024*2)


typedef struct voice_buf_t {
    unsigned int   length;
    unsigned int   index;
    unsigned char* pdata;
    unsigned char  done;
} voice_buf;

/*
 * Henry Zhang - define structure and enum for device_read/device_write operations
 */
#define CMD_BUF_LEN	1024


enum IM501_FW_TYPE {
    IM501_DSP_FW,	//MSB
    IM501_EFT_FW,	//LSB
};
/*
 * The structure dev_cmd_short/long defines the command protocol between the library and the device driver.
 * In device driver, the device read and device write functions handle the structure data and parse it.
 *
 * dev_cmd_short: the structure for device command FM_SMVD_REG_READ, FM_SMVD_REG_WRITE, FM_SMVD_DSP_READ,
 * FM_SMVD_DSP_WRITE, FM_SMVD_MODE_SET and FM_SMVD_MODE_GET, for which no extra data buffer is needed.
 */
typedef struct dev_cmd_short_t {
    unsigned short cmd_name;	//The commands from #0~#5
    unsigned int addr;			//The address of the register or dsp memory for the commands #0~#3, or the dsp mode for #4~#5.
    unsigned int val;			//The operation or returned value for the commands #0~#3, or zero for the commands #4~#5.
    unsigned char reserved[6];
} dev_cmd_short;
/*
 * dev_cmd_long: the structure for device command FM_SMVD_DSP_BWRITE, FM_SMVD_VECTOR_GET and FM_SMVD_REG_DUMP,
 * for which the extra data buffer is necessary for input or output data.
 */
typedef struct dev_cmd_long_t {
    unsigned short cmd_name;	//The command from #6~#8
    unsigned int addr;			//The address of dsp memory for the command #6, or zero for #7~#8.
    unsigned int val;			//The the valid data length.
    unsigned char reserved[6];
    unsigned char buf[CMD_BUF_LEN];	//The data buffer in fixed size, for input and output.
} dev_cmd_long;
//Henry Zhang - end of device_read/device_write operation definitions

enum {
    IM501_I2C_CMD_16_WRITE = 1,
    IM501_I2C_CMD_32_READ,
    IM501_I2C_CMD_32_WRITE,
};

enum DSP_MODE {
    FM_SMVD_DSP_BYPASS,		//the bypass mode of the dsp. in this mode the DMIC input is bypassed to codec.
    FM_SMVD_DSP_DETECTION,
    FM_SMVD_DSP_MIXTURE,
    FM_SMVD_DSP_FACTORY,
    FM_SMVD_DSP_VR,			//the voice recognition mode of the dsp.
    FM_SMVD_DSP_CM,			//the communication mode of the dsp.
    FM_SMVD_DSP_BARGE_IN,	//the barge-in mode of the dsp. in this mode FM_SMVD detects the keyword and issues interrupt to the host AP.
    FM_SMVD_GET_DSP_MODE,
    FM_SMVD_DOWNLOAD_UDT_FIRMWARE,
    FM_SMVD_DOWNLOAD_EFT_FIRMWARE,
    FM_SMVD_DOWNLOAD_WHOLE_FIRMWARE,
    FM_SMVD_SET_EFT_SVTHD,
    FM_SMVD_SET_UDT_SVTHD,
    FM_SMVD_DUMP_REGISTER,
};

typedef struct dev_cmd_mode_gs_t {
    unsigned short cmd_name;
    unsigned int dsp_mode;
    unsigned char hd_reserved[10];
} dev_cmd_mode_gs;


typedef struct dev_cmd_reg_rw_t {
    unsigned short cmd_name;
    unsigned int reg_addr;
    unsigned int reg_val;
    unsigned char hd_reserved[6];
} dev_cmd_reg_rw;

typedef struct dev_cmd_fwdl_t {
    unsigned short cmd_name;
    unsigned int dsp_addr;
    unsigned int data_len;
    unsigned char hd_reserved[6];
    unsigned char buf[1024];
} dev_cmd_fwdl;





#endif /* _IM501_H_ */
