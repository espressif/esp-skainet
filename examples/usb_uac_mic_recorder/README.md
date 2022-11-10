## Overview

This project is to transfer the on-board microphone data to the computer via the USB audio device function.

Supported Development Boards

* ESP-BOX 
* ESP-Korvo2: You need to remove the 0-ohm resistors (R181,R182) at USB_DM and USB_DP and add two 0-ohm resistors (R394,R395) at ESP_USB_DM and ESP_USB_DP. [Hardware Schematic](https://dl.espressif.com/dl/schematics/SCH_ESP32-S3-KORVO-2_V3_0_20210918.pdf)

## How To USE

1. The number of mic winds needs to be configured inside menuconfig. Currently only support 16k sample rate and 16bit bit depth.

2. You need to call `` ringbuf_handle_t mic_recorder_init()`` to initialize the usb function.

3. Use rb_write to cache audio data.

## Phenomenon

When you plug the usb port into your computer(**Linux only**), it will display the MicNode sound card device.