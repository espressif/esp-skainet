# ESP-Skainet [[English]](./README.md)

ESP-Skainet 是乐鑫推出的智能语音助手，目前支持唤醒词识别和语音命令词识别。

### 推荐使用 ESP32-S3 运行语音命令词识别，因其支持 AI 指令和高速八线 SPI PSRAM。最新模型将优先在 ESP32-S3 上部署。

# 概述

ESP-Skainet 以最便捷的方式支持基于乐鑫 ESP32 系列芯片的唤醒词识别和语音命令词识别应用程序开发。使用 ESP-Skainet，您可以轻松构建唤醒词和命令词识别应用。

ESP-Skainet 主要功能如下：

![overview](img/skainet_overview2.png)

## 输入音频

输入音频流可以来自麦克风，或 flash/SD 卡中的 wav/pcm 等音频文件。

## 唤醒词识别

唤醒词模型 [WakeNet](https://docs.espressif.com/projects/esp-sr/zh_CN/latest/esp32s3/wake_word_engine/README.html) 专为用户提供高性能、低资源消耗的唤醒词检测算法，使设备能够持续待命，等待如“Alexa”、“天猫精灵”、“小爱同学”等唤醒词。

目前乐鑫免费开放“Hi，乐鑫”、“你好小智”、“你好小鑫”、“Hi，ESP”唤醒词。如果用户需要其他唤醒词，乐鑫也提供定制服务，详见 [乐鑫语音唤醒词定制流程](https://docs.espressif.com/projects/esp-sr/zh_CN/latest/esp32/wake_word_engine/ESP_Wake_Words_Customization.html)。

## 语音命令词识别

命令词识别模型 [MultiNet](https://docs.espressif.com/projects/esp-sr/zh_CN/latest/esp32s3/speech_command_recognition/README.html) 专为提供灵活的离线语音命令词识别而设计。用户可根据需求自定义语音命令，无需重新训练模型。

目前 **MultiNet** 支持最多 200 个中英文命令词，如“打开空调”、“打开卧室灯”或 "Turn on the light" 等。

## 声学前端算法

声学前端 [AFE](https://docs.espressif.com/projects/esp-sr/zh_CN/latest/esp32s3/audio_front_end/index.html) 集成了回声消除（AEC）、语音活动检测（VAD）、盲源分离（BSS）和噪声抑制（NS）等算法。

我们的双麦克风声学前端（AFE）已通过 [Amazon Alexa Built-in 设备的软件音频前端解决方案](https://developer.amazon.com/en-US/alexa/solution-providers/dev-kits#software-audio-front-end-dev-kits)认证。
![afe](img/esp_afe.png)

# 快速开始

## 硬件准备

要运行 ESP-Skainet，您需要一块集成了音频输入模块的 ESP32 或 ESP32-S3 开发板。

|                          Example Name                               |   Latest Models   |  Supported Board   |
| :------------------------------------------------------------------ | :---------------: | :-------------- |
| [cn_speech_commands_recognition](./cn_speech_commands_recognition) | MultiNet7      | [ESP32-Korvo](https://github.com/espressif/esp-skainet/blob/master/docs/en/hw-reference/esp32/user-guide-esp32-korvo-v1.1.md), [ESP32-S3-Korvo-1](https://github.com/espressif/esp-skainet/blob/master/docs/en/hw-reference/esp32s3/user-guide-korvo-1.md), [ESP-BOX](https://github.com/espressif/esp-box), [ESP-S3-Korvo-2](https://docs.espressif.com/projects/esp-adf/en/latest/get-started/user-guide-esp32-s3-korvo-2.html), [ESP32-S3-EYE](https://www.espressif.com/en/products/devkits/esp-s3-eye/overview), [ESP32-P4-Function-EV](https://docs.espressif.com/projects/esp-dev-kits/en/latest/esp32p4/esp32-p4-function-ev-board/user_guide.html#getting-started)|
| [en_speech_commands_recognition](./en_speech_commands_recognition) | MultiNet7      | [ESP32-S3-Korvo-1](https://github.com/espressif/esp-skainet/blob/master/docs/en/hw-reference/esp32s3/user-guide-korvo-1.md), [ESP-BOX](https://github.com/espressif/esp-box), [ESP-S3-Korvo-2](https://docs.espressif.com/projects/esp-adf/en/latest/get-started/user-guide-esp32-s3-korvo-2.html), [ESP32-S3-EYE](https://www.espressif.com/en/products/devkits/esp-s3-eye/overview), [ESP32-P4-Function-EV](https://docs.espressif.com/projects/esp-dev-kits/en/latest/esp32p4/esp32-p4-function-ev-board/user_guide.html#getting-started)|
| [wake_word_detection](./wake_word_detection)                       | WakeNet9       | [ESP32-Korvo](https://github.com/espressif/esp-skainet/blob/master/docs/en/hw-reference/esp32/user-guide-esp32-korvo-v1.1.md), [ESP32-S3-Korvo-1](https://github.com/espressif/esp-skainet/blob/master/docs/en/hw-reference/esp32s3/user-guide-korvo-1.md), [ESP-BOX](https://github.com/espressif/esp-box), [ESP-S3-Korvo-2](https://docs.espressif.com/projects/esp-adf/en/latest/get-started/user-guide-esp32-s3-korvo-2.html), [ESP32-S3-EYE](https://www.espressif.com/en/products/devkits/esp-s3-eye/overview), [ESP32-P4-Function-EV](https://docs.espressif.com/projects/esp-dev-kits/en/latest/esp32p4/esp32-p4-function-ev-board/user_guide.html#getting-started)|
| [chinese_tts](./chinese_tts)                                       | esp-tts-v1.7    | [ESP32-Korvo](https://github.com/espressif/esp-skainet/blob/master/docs/en/hw-reference/esp32/user-guide-esp32-korvo-v1.1.md), [ESP32-S3-Korvo-1](https://github.com/espressif/esp-skainet/blob/master/docs/en/hw-reference/esp32s3/user-guide-korvo-1.md), [ESP-BOX](https://github.com/espressif/esp-box), [ESP-S3-Korvo-2](https://docs.espressif.com/projects/esp-adf/en/latest/get-started/user-guide-esp32-s3-korvo-2.html), [ESP32-P4-Function-EV](https://docs.espressif.com/projects/esp-dev-kits/en/latest/esp32p4/esp32-p4-function-ev-board/user_guide.html#getting-started)|
| [usb_mic_recorder](./usb_mic_recorder)                                       |      | [ESP-BOX](https://github.com/espressif/esp-box), [ESP-S3-Korvo-2](https://docs.espressif.com/projects/esp-adf/en/latest/get-started/user-guide-esp32-s3-korvo-2.html)|

关于应用的具体配置，请参考每个示例中的 README.md。

## 软件准备

### ESP-Skainet

请使用以下命令克隆本项目：
```
git clone https://github.com/espressif/esp-skainet.git
```

### ESP-IDF

本工程支持 [ESP-IDF v4.4](https://github.com/espressif/esp-idf/tree/release/v4.4) 和 [ESP-IDF v5.0](https://github.com/espressif/esp-idf/tree/release/v5.0) 版本。如果您已搭建 ESP-IDF 环境且不想更改现有配置，可将 `IDF_PATH` 环境变量指向新的 ESP-IDF 路径。

### 注意：如需使用 ESP-IDF v3.2 及更早版本，请参考 esp-skainet v0.2.0。

更多搭建 ESP-IDF 环境的细节请参考 [ESP-IDF v4.4 快速入门指南](https://docs.espressif.com/projects/esp-idf/zh_CN/release-v4.4/esp32s3/get-started/index.html)

# 示例

[examples](examples) 文件夹包含了 ESP-Skainet API 的应用示例。

建议从 [wake_word_detection](./examples/wake_word_detection) 示例开始：

1. 进入示例文件夹：
```
cd esp-skainet/examples/wake_word_detection
```
2. 编译和烧录：
```
idf.py flash monitor
```
3. 高级用户可通过 `idf.py menuconfig` 命令添加或修改语音命令。

更多细节请阅读每个示例中的 README 文件。

# 资源

* [在 GitHub Issues 区反馈问题](https://github.com/espressif/esp-skainet/issues)：如发现 bug 或有新需求，请先查阅现有问题再提交。

* 如果您有兴趣参与 ESP-Skainet 开发，请参考 [贡献指南](https://docs.espressif.com/projects/esp-idf/zh_CN/latest/esp32/contribute/index.html)。
