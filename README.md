# ESP-Skainet [[中文]](./README_cn.md)

ESP-Skainet is Espressif's intelligent voice assistant, currently supporting wake word detection and speech command recognition.

### It is recommended to use ESP32-S3 for speech command recognition, as it supports AI instructions and high-speed octal SPI PSRAM. The latest models will be deployed on ESP32-S3 first.

# Overview

ESP-Skainet enables convenient development of wake word detection and speech command recognition applications based on Espressif's ESP32 series chips. With ESP-Skainet, you can easily build wake word and command recognition solutions.

ESP-Skainet has the following features:

![overview](img/skainet_overview2.png)

## Input Voice Stream

The input audio stream can come from a microphone or from wav/pcm files stored in flash or SD card.

## Wake Word Engine

The [WakeNet](https://docs.espressif.com/projects/esp-sr/en/latest/esp32s3/wake_word_engine/README.html) engine is designed to provide high performance and low memory usage for wake word detection, enabling devices to always listen for wake words such as "Alexa", “天猫精灵” (Tian Mao Jing Ling), and “小爱同学” (Xiao Ai Tong Xue).

Espressif provides wake words such as "Hi, Lexin" and "Hi, ESP" for free, and also supports custom wake words. For details, see the [Espressif Speech Wake Words Customization Process](https://docs.espressif.com/projects/esp-sr/en/latest/esp32/wake_word_engine/ESP_Wake_Words_Customization.html).

## Speech Commands Recognition

The [MultiNet](https://docs.espressif.com/projects/esp-sr/en/latest/esp32s3/speech_command_recognition/README.html) model provides flexible offline speech command recognition. You can easily add your own commands without retraining the model.

Currently, **MultiNet** supports up to 200 Chinese or English speech commands, such as “打开空调” (Turn on the air conditioner) and “打开卧室灯” (Turn on the bedroom light).

## Audio Front End

The [Audio Front-End (AFE)](https://docs.espressif.com/projects/esp-sr/en/latest/esp32s3/audio_front_end/index.html) integrates AEC (Acoustic Echo Cancellation), VAD (Voice Activity Detection), BSS (Blind Source Separation), and NS (Noise Suppression).

Our two-mic AFE has been qualified as a “Software Audio Front-End Solution” for [Amazon Alexa Built-in devices](https://developer.amazon.com/en-US/alexa/solution-providers/dev-kits#software-audio-front-end-dev-kits).
![AFE](img/esp_afe.png)

# Quick Start with ESP-Skainet

## Hardware Preparation

To run ESP-Skainet, you need an ESP32 or ESP32-S3 development board with an integrated audio input module.

|                          Example Name                               |   Latest Models   |  Supported Board   |
| :------------------------------------------------------------------ | :---------------: | :-------------- |
| [cn_speech_commands_recognition](./cn_speech_commands_recognition) | MultiNet7      | [ESP32-Korvo](https://github.com/espressif/esp-skainet/blob/master/docs/en/hw-reference/esp32/user-guide-esp32-korvo-v1.1.md), [ESP32-S3-Korvo-1](https://github.com/espressif/esp-skainet/blob/master/docs/en/hw-reference/esp32s3/user-guide-korvo-1.md), [ESP-BOX](https://github.com/espressif/esp-box), [ESP-S3-Korvo-2](https://docs.espressif.com/projects/esp-adf/en/latest/get-started/user-guide-esp32-s3-korvo-2.html), [ESP32-S3-EYE](https://www.espressif.com/en/products/devkits/esp-s3-eye/overview), [ESP32-P4-Function-EV](https://docs.espressif.com/projects/esp-dev-kits/en/latest/esp32p4/esp32-p4-function-ev-board/user_guide.html#getting-started)|
| [en_speech_commands_recognition](./en_speech_commands_recognition) | MultiNet7      | [ESP32-S3-Korvo-1](https://github.com/espressif/esp-skainet/blob/master/docs/en/hw-reference/esp32s3/user-guide-korvo-1.md), [ESP-BOX](https://github.com/espressif/esp-box), [ESP-S3-Korvo-2](https://docs.espressif.com/projects/esp-adf/en/latest/get-started/user-guide-esp32-s3-korvo-2.html), [ESP32-S3-EYE](https://www.espressif.com/en/products/devkits/esp-s3-eye/overview), [ESP32-P4-Function-EV](https://docs.espressif.com/projects/esp-dev-kits/en/latest/esp32p4/esp32-p4-function-ev-board/user_guide.html#getting-started)|
| [wake_word_detection](./wake_word_detection)                       | WakeNet9       | [ESP32-Korvo](https://github.com/espressif/esp-skainet/blob/master/docs/en/hw-reference/esp32/user-guide-esp32-korvo-v1.1.md), [ESP32-S3-Korvo-1](https://github.com/espressif/esp-skainet/blob/master/docs/en/hw-reference/esp32s3/user-guide-korvo-1.md), [ESP-BOX](https://github.com/espressif/esp-box), [ESP-S3-Korvo-2](https://docs.espressif.com/projects/esp-adf/en/latest/get-started/user-guide-esp32-s3-korvo-2.html), [ESP32-S3-EYE](https://www.espressif.com/en/products/devkits/esp-s3-eye/overview), [ESP32-P4-Function-EV](https://docs.espressif.com/projects/esp-dev-kits/en/latest/esp32p4/esp32-p4-function-ev-board/user_guide.html#getting-started)|
| [chinese_tts](./chinese_tts)                                       | esp-tts-v1.7    | [ESP32-Korvo](https://github.com/espressif/esp-skainet/blob/master/docs/en/hw-reference/esp32/user-guide-esp32-korvo-v1.1.md), [ESP32-S3-Korvo-1](https://github.com/espressif/esp-skainet/blob/master/docs/en/hw-reference/esp32s3/user-guide-korvo-1.md), [ESP-BOX](https://github.com/espressif/esp-box), [ESP-S3-Korvo-2](https://docs.espressif.com/projects/esp-adf/en/latest/get-started/user-guide-esp32-s3-korvo-2.html), [ESP32-P4-Function-EV](https://docs.espressif.com/projects/esp-dev-kits/en/latest/esp32p4/esp32-p4-function-ev-board/user_guide.html#getting-started)|
| [usb_mic_recorder](./usb_mic_recorder)                                       |      | [ESP-BOX](https://github.com/espressif/esp-box), [ESP-S3-Korvo-2](https://docs.espressif.com/projects/esp-adf/en/latest/get-started/user-guide-esp32-s3-korvo-2.html)|

For application configuration, please refer to the README.md in each example folder.

## Software Preparation

### ESP-Skainet

Clone this project:
```
git clone https://github.com/espressif/esp-skainet.git
```

### ESP-IDF

[ESP-IDF v4.4](https://github.com/espressif/esp-idf/tree/release/v4.4) and [ESP-IDF v5.0](https://github.com/espressif/esp-idf/tree/release/v5.0) are supported. If you have already set up ESP-IDF and do not want to change your existing environment, you can set the `IDF_PATH` environment variable to the new ESP-IDF path.

### Note: If you need to use ESP-IDF v3.2 or earlier, please refer to esp-skainet v0.2.0.

For setup details, see the [Getting Started Guide for ESP-IDF v4.4](https://docs.espressif.com/projects/esp-idf/en/release-v4.4/esp32s3/get-started/index.html).

# Examples

The [examples](examples) folder contains sample applications using the ESP-Skainet API.

We recommend starting with the [wake_word_detection](./examples/wake_word_detection) example:

1. Enter the example folder:
	```
	cd examples/wake_word_detection
	```
2. Compile and flash the project:
	```
	idf.py flash monitor
	```
3. Advanced users can add or modify speech commands using:
	```
	idf.py menuconfig
	```

For more details, see the README in each example folder.

# Resources

* [View the Issues section on GitHub](https://github.com/espressif/esp-skainet/issues) — If you find a bug or have a feature request, please check existing issues before opening a new one.
* Interested in contributing? See the [Contributions Guide](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/contribute/index.html).
