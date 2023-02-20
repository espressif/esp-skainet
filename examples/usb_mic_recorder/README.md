## Overview

This project is to transfer the on-board microphone data to the computer via the USB audio device function.

Supported Development Boards

* ESP-BOX 
* ESP32-S3-Korvo-2: You need to remove the 0-ohm resistors (R181,R182) at USB_DM and USB_DP and add two 0-ohm resistors (R394,R395) at ESP_USB_DM and ESP_USB_DP. [Hardware Schematic](https://dl.espressif.com/dl/schematics/SCH_ESP32-S3-KORVO-2_V3_0_20210918.pdf)

## How To USE

### 1. compile and flash project
```
idf.py flash
```

Note: If you use USB for burning. Holding down `Boot` and then pressing `Reset` initiates Firmware Download mode for downloading firmware through the serial port.

### 2. select sound card device

1. When you plug the usb port into your computer(**Linux only**), it will display the MicNode sound card device. Please select MicNode sound card device.
![MicNode](../../img/MicNode.png)

2. Use linux terminal

    ```bash
    #first
    sudo apt-get install alsa-utils alsa-tools alsa-tools-gui alsamixergui -y
    #second
    arecord -l
    #third
    arecord -d 60 -f cd -r 48000 -c 3 -t wav voice.wav
    #for more
    arecord -h
    ```