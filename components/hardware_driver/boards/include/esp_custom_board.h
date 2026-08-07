#pragma once
#include "driver/gpio.h"

// I2S RX — microphone (INMP441-style digital mic, LR pin tied to GND)
#define GPIO_I2S_SCLK   GPIO_NUM_4    // mic CK / BCLK
#define GPIO_I2S_LRCK   GPIO_NUM_5    // mic WS
#define GPIO_I2S_SDIN   GPIO_NUM_6    // mic DA

// I2S TX — MAX98357A speaker amp (SD pin tied to 3.3V)
#define GPIO_I2S0_SCLK  GPIO_NUM_7    // amp BCLK
#define GPIO_I2S0_LRCK  GPIO_NUM_15   // amp LRC
#define GPIO_I2S0_DOUT  GPIO_NUM_16   // amp DIN

// No I2C codec, no SD card on this board
#define FUNC_SDMMC_EN   0
#define FUNC_SDSPI_EN   0
