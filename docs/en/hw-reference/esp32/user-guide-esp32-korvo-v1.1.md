# ESP32-Korvo V1.1 User Guide

* [中文版](../../../zh_CN/hw-reference/esp32/user-guide-esp32-korvo-v1.1.md)

This user guide provides information on ESP32-Korvo V1.1.

ESP32-Korvo V1.1 is a dual-core ESP32-based audio development board with microphone arrays. Together with Espressif's speech recognition SDK ESP-Skainet, ESP32-Korvo V1.1 is suitable for developing far-field speech recognition applications with low power consumption, such as smart displays, smart plugs, smart switches, etc.

![ESP32-Korvo V1.1](../../../_static/esp32-korvo-v1.1-front-view.png)
<div align=center>ESP32-Korvo V1.1</div>

The document consists of the following major sections:

-   [Getting started](#1-getting-started): Provides an overview of ESP32-Korvo V1.1 and hardware/software setup instructions to get started.
-   [Hardware reference](#2-hardware-reference): Provides more detailed information about the ESP32-Korvo V1.1's hardware.
-   [Related Documents](#3-related-documents): Gives links to related documentaiton.

# 1. Getting Started

This section describes how to get started with ESP32-Korvo V1.1. It begins with a few introductory sections about ESP32-Korvo V1.1, then Section [Start Application Development](#14-start-application-development) provides instructions on how to do the initial hardware setup and then how to flash firmware into ESP32-Korvo V1.1.

## 1.1 Overview

ESP32-Korvo V1.1 consists of two boards connected by an FPC cable: one mainboard containing an ESP32-WROVER-E module, a USB power port, a micro SD card slot, an earphone connector and a speaker connector, and one subboard containing microphone arrays, function buttons and RGB LEDs.

Unlike other Espressif's audio development boards, ESP32-Korvo V1.1 achieves far-field offline speech recoginition with a single ESP32 chip. The board can be equipped with either two or three microphone arrays. Combined with beamforming, AEC and other speech enhancement algorithms, it applies to various speech-recognition scenarios. 

## 1.2  Contents and Packaging

### 1.2.1 Retail orders

If you order one or several samples, each ESP32-Korvo V1.1 comes in an individual package which would contain:

* ESP32-Korvo V1.1 mainboard
* ESP32-Korvo V1.1 subboard
* FPC cable
* 8 screws
* 4 studs

For retail orders, please go to <https://www.espressif.com/en/company/contact/buy-a-sample>.

### 1.2.2 Wholesale Orders

If you order in bulk, the boards come in large cardboard boxes.

For wholesale orders, please check [Espressif Product Ordering Information](https://www.espressif.com/sites/default/files/documentation/espressif_products_ordering_information_en.pdf) (PDF).

## 1.3 Description of Components

![ESP32-Korvo V1.1 - mainboard front](../../../_static/esp32-korvo-v1.1-annotated-photo-mainboard.png)
<div align=center>ESP32-Korvo V1.1 Mainboard Front</div>

![ESP32-Korvo V1.1 - mic front](../../../_static/esp32-korvo-v1.1-annotated-photo-mic-front.png)
<div align=center>ESP32-Korvo V1.1 Subboard Front</div>

![ESP32-Korvo V1.1 - mic back](../../../_static/esp32-korvo-v1.1-annotated-photo-mic-back.png)
<div align=center>ESP32-Korvo V1.1 Subboard Back</div>

|Key Componenet|Description|
|:- |:- |
|ESP32-WROVER-E|This ESP32 module contains the latest ESP32-D0WD-V3, a 16 MB flash and a 8 MB PSRAM for flexible data storage, featuring Wi-Fi / BT connectivity and data processing capability.|
|Power Regulator|5V-to-3.3V regulator.|
|Power Switch|On: The board is powered on; Off: The board is powered off.|
|Battery Port|Connect a battery.|
|USB Power Port|Supply power to the board.|
|USB-UART Port|A communication interface between a computer and the ESP32-WROVER-E module.|
|USB-UART Bridge|Single USB-UART bridge chip provides transfer rates of up to 3 Mbps.| check
|Reset Button|Pressing this button resets the system.|
|Boot Button|Download button. Holding down Boot and then pressing EN initiates Firmware Download mode for downloading firmware through the serial port.|
|Micro SD Card Slot| Useful for developing applications that access MicroSD card for data storage and retrieval.|
|Audio ADC| High-performance four-channel audio ADC. Among the four channels, three are for microphones, and one for AEC function.|
|Audio PA|Amplify audio signals to external speaker at maximum 3 W.|
|Speaker Connector|Connect an external speaker.|
|Earphone Connector|Connect external earphones.|
|FPC Connector|Connect mainboard and subboard.|
|Audio Codec|Audio codec ES8311 communicates with ESP32 via the I2S bus, which converts digital signals to analog signals.|
|Analog Microphone|Three analog microphone arrays (spacing = 65mm).|
|RGB LED|12 addressable RGB LEDs (WS2812).|
|Function Button|Six function buttons, i.e. PLAY, SET, VOL -, VOL +, MODE and REC. These function buttons are user-definable.|

## 1.4 Default Firmware

[The default firmware](https://github.com/espressif/esp-skainet/blob/master/tools/default_firmware/esp32_korvo_v1_1_fw_v0_1_20200323.bin) in ESP32-Korvo V1.1 allows you to experience voice wake-up and speech command recognition with on-board RGB LEDs.

After powering up your ESP32-Korvo 1.1 (mainboard and subboard connected by the FPC cable) and pressing the Reset button, please activate the board with the default wake word “Hi, Le Xin”, which translates in English as “Hello, Espressif”. When the wake word is detected, the 12 on-board RGB LEDs glow white one by one, meaning that the board is waiting for a speech command.

|Default Speech Command (in Chinese)|Meaning|Response|
|:- |:- |:- |
|Guan Bi Dian Deng|Turn off the light|RGB LEDs go out.|
|Da Kai Bai Deng|Turn on the white light|RGB LEDs glow white.|
|Da Kai Hong Deng|Turn on the red light|RGB LEDs glow red.|
|Da Kai Lv Deng|Turn on the green light|RGB LEDs glow green.|
|Da Kai Lan Deng|Turn on the blue light|RGB LEDs glow blue.|
|Da Kai Huang Deng|Turn on the yellow light|RGB LEDs glow yellow.|
|Da Kai Cheng Deng|Turn on the orange light|RGB LEDs glow orange.|
|Da Kai Zi Deng|Turn on the purple light|RGB LEDs glow purple.|

If the command is not recognized, RGB LEDs returns to the state before voice wake-up.

## 1.4 Start Application Development

Before powering up your ESP32-Korvo V1.1, please make sure that it is in good condition with no obvious signs of damage.

### 1.4.1 Required Hardware

* ESP32-Korvo V1.1
* 2 x USB 2.0 cable (Standard-A to Micro-B)
* 4-ohm speaker or earphones
* Computer running Windows, Linux, or macOS

### 1.4.2 Hardware Setup

1. Connect ESP32-Korvo V1.1 subboard to ESP32-Korvo V1.1 mainboard through the FPC cable.
2. Connect a 4-ohm speaker to the Speaker Connector, or connect earphones to the Earphone Connector.
3. Plug in the USB cables to the PC and to both USB ports of ESP32-Korvo V1.1.
4. Turn on the Power Switch.
5. The Power On LED (Red) should turn on.

### 1.4.3 Software Setup

After hardware setup, you can proceed with preparation of development tools. Go to section [Software Preparation](https://github.com/espressif/esp-skainet/blob/master/README.md#esp-skainet-中文), which will walk you through the following steps:

1. [Set up ESP-IDF](https://github.com/espressif/esp-skainet/blob/master/README.md#esp-idf) which provides a common framework to develop applications for ESP32 in C language.
2. [Get ESP-Skainet](https://github.com/espressif/esp-skainet/blob/master/README.md#esp-skainet) to run Espressif's voice assistant. In ESP-Skainet, you may use [ESP-SR](https://github.com/espressif/esp-skainet/blob/master/README.md#esp-sr) to call APIs for specific applications, including wake word detection, speech command recognition and acoustic algorithm.
4. [Build, flash and run ESP-Skainet examples](https://github.com/espressif/esp-skainet/blob/master/README.md#examples).

# 2. Hardware Reference

## 2.1 Block Diagram

A block diagram below shows the components of ESP32-Korvo V1.1 and their interconnections.

![ESP32-Korvo V1.1 Block Diagram](../../../_static/esp32-korvo-v1.1-block-diagram.png)
<div align=center>ESP32-Korvo V1.1 Block Diagram</div>

# 3. Related Documents

* [ESP32-Korvo V1.1 Mainboard Schematics](https://dl.espressif.com/dl/schematics/ESP32-KORVO_V1.1_schematics.pdf) (PDF)
* [ESP32-Korvo V1.1 Subboard Schematics](https://dl.espressif.com/dl/schematics/ESP32-KORVO-MIC_V1.1_schematics.pdf) (PDF)
* [ESP32-Korvo V1.1 Mainboard PCB Layout](https://dl.espressif.com/dl/schematics/ESP32-Korvo-Mainboard_V1.1_PCB_Layout.pdf) (PDF)
* [ESP32-Korvo V1.1 Subboard PCB Layout](https://dl.espressif.com/dl/schematics/ESP32-Korvo-Mic_V1.1_PCB_Layout.pdf) (PDF)
* [ESP32 Datasheet](https://www.espressif.com/sites/default/files/documentation/esp32_datasheet_en.pdf) (PDF)
* [ESP32-WROVER-E & ESP32-WROVER-IE Datasheet](https://www.espressif.com/sites/default/files/documentation/esp32-wrover-e_esp32-wrover-ie_datasheet_en.pdf) (PDF)
* [Espressif Product Ordering Information](https://www.espressif.com/sites/default/files/documentation/espressif_products_ordering_information_en.pdf) (PDF)
