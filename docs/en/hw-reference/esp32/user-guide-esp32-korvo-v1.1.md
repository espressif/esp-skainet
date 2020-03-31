# ESP32-Korvo User Guide

* [中文版](../../../zh_CN/hw-reference/esp32/user-guide-esp32-korvo-v1.1.md)

This user guide provides information on ESP32-Korvo.

ESP32-Korvo is a dual-core ESP32-based audio development board with microphone arrays. Together with Espressif's speech recognition SDK ESP-Skainet, ESP32-Korvo is suitable for far-field speech recognition applications with low power consumption, such as smart displays, smart plugs, smart switches, etc.

|![ESP32-Korvo](../../../_static/esp32-korvo-v1.1-isometric.png)|![ESP32-Korvo Mic](../../../_static/esp32-korvo-v1.1-isometric-mic.png)|
|:- |:- |
|ESP32-Korvo|ESP32-Korvo Mic|

The document consists of the following major sections:

-   [Getting started](#1-getting-started): Provides an overview of ESP32-Korvo and hardware/software setup instructions to get started.
-   [Hardware reference](#2-hardware-reference): Provides more detailed information about the ESP32-Korvo's hardware.
-   [Related Documents](#3-related-documents): Gives links to related documentaiton.

# 1. Getting Started

This section describes how to get started with ESP32-Korvo. It begins with a few introductory sections about ESP32-Korvo, then Section [Start Application Development](#14-start-application-development) provides instructions on how to do the initial hardware setup and then how to flash firmware into ESP32-Korvo.

## 1.1 Overview

ESP32-Korvo is a dual-core ESP32-based audio development board with microphone arrays. Together with Espressif's speech recognition SDK ESP-Skainet, ESP32-Korvo is suitable for far-field speech recognition applications with low power consumption, such as smart displays, smart plugs, smart switches, etc.

Unlike other Espressif's audio development boards, ESP32-Korvo achieves far-field offline speech recoginition with a single ESP32 chip. The board can be equipped with either two or three microphone arrays. Combined with beamforming, AEC and other speech enhancement algorithms, it applies to various speech-recognition scenarios. 

## 1.2  Contents and Packaging

### 1.2.1 Retail orders

If you order one or several samples, each ESP32-Korvo comes in an individual package which would contain:
* ESP32-Korvo mainboard
* ESP32-Korvo Mic board
* FPC cable
* 8 screws
* 4 studs

For retail orders, please go to <https://www.espressif.com/en/company/contact/buy-a-sample>.

### 1.2.2 Wholesale Orders

If you order in bulk, the baords come in large cardboard boxes.

For wholesale orders, please check [Espressif Product Ordering Information](https://www.espressif.com/sites/default/files/documentation/espressif_products_ordering_information_en.pdf) (PDF).

## 1.3 Description of Components

![ESP32-Korvo - mainboard front](../../../_static/esp32-korvo-v1.1-annotated-photo.png)

![ESP32-Korvo - mic front](../../../_static/esp32-korvo-v1.1-annotated-photo-mic-front.png)

![ESP32-Korvo - mic back](../../../_static/esp32-korvo-v1.1-annotated-photo-mic-back.png)

|Key Componenet|Description|
|:- |:- |
|ESP32-WROVER-B|This ESP32 module contains an ESP32 chip to provide Wi-Fi / BT connectivity and data processing power. It integrates 64 Mbit SPI flash and 128 Mbit PSRAM for flexible data storage.|
|Power Regulator|5V-to-3.3V regulator.|
|Power Switch|On: The board is powered on; Off: The board is powered off.|
|Battery Port|Connect a battery.|
|USB Power Port|Supply power to the board.|
|USB-UART Port|A communication interface between a computer and the ESP32-WROVER-B module.|
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
|RGB LED|12 addressable RGB LEDs (WS2812).|
|Function Button|Six function buttons, i.e. PLAY, SET, VOL -, VOL +, MODE and REC. These function buttons are user-definable.|
|Analog Microphone|Six analog microphone arrays (spacing = 65mm).|
  
## 1.4 Start Application Development

Before powering up your ESP32-Korvo, please make sure that it is in good condition with no obvious signs of damage.

### 1.4.1 Required Hardware

* ESP32-Korvo
* 2 x USB 2.0 cable (Standard-A to Micro-B)
* 4-ohm speaker or earphones
* Computer running Windows, Linux, or macOS

### 1.4.2 Hardware Setup

1. Connect ESP32-Korvo Mic board to ESP32-Korvo mainboard through the FPC cable.
2. Connect a 4-ohm speaker to the Speaker Connector, or connect earphones to the Earphone Connector.
3. Plug in the USB cables to the PC and to both USB ports of ESP32-Korvo.
4. Turn on the Power Switch.
5. The Power On LED (Red) should turn on.

### 1.4.3 Software Setup

After hardware setup, you can proceed with preparation of development tools. Go to section [Software Preparation](https://github.com/espressif/esp-skainet/blob/master/README.md#esp-skainet-中文), which will walk you through the following steps:

* [Set up ESP-IDF](https://github.com/espressif/esp-skainet/blob/master/README.md#esp-idf) which provides a common framework to develop applications for ESP32 in C language.
* [Get ESP-Skainet](https://github.com/espressif/esp-skainet/blob/master/README.md#esp-skainet) to run Espressif's voice assistant.
* [Get ESP-SR](https://github.com/espressif/esp-skainet/blob/master/README.md#esp-sr) to call APIs specific for ESP-Skainet applications, including wake work detection, speech command recognition and acoustic algorithm.
* [Build, flash and run ESP-Skainet examples](https://github.com/espressif/esp-skainet/blob/master/README.md#examples).

# 2. Hardware Reference

## 2.1 Block Diagram

A block diagram below shows the components of ESP32-Korvo and their interconnections.

![ESP32-Korvo](../../../_static/esp32-korvo-v1.1-block-diagram.png)

# 3. Related Documents

* [ESP32-Korvo Mainboard Schematics](https://dl.espressif.com/dl/schematics/ESP32-KORVO_V1.1_schematics.pdf) (PDF)
* [ESP32-Korvo Mic Schematics](https://dl.espressif.com/dl/schematics/ESP32-KORVO-MIC_V1.1_schematics.pdf) (PDF)
* [ESP32-Korvo Reference Design]() (ZIP)
* [ESP32 Datasheet](https://www.espressif.com/sites/default/files/documentation/esp32_datasheet_en.pdf) (PDF)
* [ESP32-WROVER-B Datasheet](https://www.espressif.com/sites/default/files/documentation/esp32-wrover-b_datasheet_en.pdf)(PDF)
* [ESP32-WROVER-B-V1 Reference Design](https://www.espressif.com/sites/default/files/documentation/ESP32-WROVER-B_Reference_Design_V1-r1.0_0.zip) (ZIP)
* [Espressif Product Ordering Information](https://www.espressif.com/sites/default/files/documentation/espressif_products_ordering_information_en.pdf) (PDF)
