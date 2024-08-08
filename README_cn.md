# ESP-Skainet [[English]](./README.md)

ESP-Skainet 是乐鑫推出的智能语音助手，目前支持唤醒词识别和命令词识别。

# 概述

ESP-Skainet 以最便捷的方式支持基于乐鑫的 ESP32系列 芯片的唤醒词识别和命令词识别应用程序的开发。使用 ESP-Skainet，您可以轻松构建唤醒词识别和命令词识别应用程序。

ESP-Skainet 的功能支持如下所示：

![overview](img/skainet_overview2.png)

## 输入音频

输入音频流可以来自麦克风，或Flash/TF 卡中的 wav/pcm 等音频文件文件。

## 唤醒词识别 

唤醒词模型 [WakeNet](https://docs.espressif.com/projects/esp-sr/zh_CN/latest/esp32s3/wake_word_engine/README.html)，致力于提供一个低资源消耗的的高性能模型，支持类似“Alexa”，“天猫精灵”，“小爱同学”等唤醒词的识别。  

目前乐鑫免费开放“Hi，乐鑫”， “你好小智”， “你好小鑫”， “Hi，ESP”唤醒词。如果用户需要其它唤醒词，乐鑫提供有唤醒词定制服务，具体可参考 [乐鑫语音唤醒词定制流程](https://docs.espressif.com/projects/esp-sr/zh_CN/latest/esp32s3/wake_word_engine/ESP_Wake_Words_Customization.html)。

## 语音命令词识别

命令词识别模型 [MultiNet](https://docs.espressif.com/projects/esp-sr/zh_CN/latest/esp32s3/speech_command_recognition/README.html) ，致力于提供一个灵活的离线语音命词识别框架。用户可方便根据需求自定义语音命令，无需重新训练模型。  

目前模型支持类似“打开空调”，“打开卧室灯”等中文命令词识别和"Turn on/off the light" 等英文命令词识别，自定义语音命令词最大个数为 200。   

## 声学前端算法

声学前端算法[Audio Front-End(AFE)](https://docs.espressif.com/projects/esp-sr/zh_CN/latest/esp32s3/audio_front_end/index.html) 集成了回声消除 AEC(Acoustic Echo Cancellation)，自动增益调节 AGC(automatic_gain_control)，噪声抑制 NS(Noise Suppression)，语音活动检测 VAD(Voice Activity Detection) 和麦克风阵列算法(Mic Array Speech Enhancement)。

# 快速开始

## 硬件准备

为了运行 ESP-Skainet，您需要一块集成了音频输入模块的开发板。
开发板支持：

The following examples are currently available:

|                          Example Name                               |   Latest Models   |  Supported Board   |
| :------------------------------------------------------------------ | :---------------: | :-------------- |
| [cn_speech_commands_recognition](./cn_speech_commands_recognition) | MultiNet7      | [ESP32-Korvo](https://github.com/espressif/esp-skainet/blob/master/docs/en/hw-reference/esp32/user-guide-esp32-korvo-v1.1.md), [ESP32-S3-Korvo-1](https://github.com/espressif/esp-skainet/blob/master/docs/en/hw-reference/esp32s3/user-guide-korvo-1.md), [ESP-BOX](https://github.com/espressif/esp-box), [ESP-S3-Korvo-2](https://docs.espressif.com/projects/esp-adf/en/latest/get-started/user-guide-esp32-s3-korvo-2.html), [ESP32-S3-EYE](https://www.espressif.com/en/products/devkits/esp-s3-eye/overview), [ESP32-P4-Function-EV](https://docs.espressif.com/projects/esp-dev-kits/en/latest/esp32p4/esp32-p4-function-ev-board/user_guide.html#getting-started)|
| [en_speech_commands_recognition](./en_speech_commands_recognition) | MultiNet7      | [ESP32-S3-Korvo-1](https://github.com/espressif/esp-skainet/blob/master/docs/en/hw-reference/esp32s3/user-guide-korvo-1.md), [ESP-BOX](https://github.com/espressif/esp-box), [ESP-S3-Korvo-2](https://docs.espressif.com/projects/esp-adf/en/latest/get-started/user-guide-esp32-s3-korvo-2.html), [ESP32-S3-EYE](https://www.espressif.com/en/products/devkits/esp-s3-eye/overview), [ESP32-P4-Function-EV](https://docs.espressif.com/projects/esp-dev-kits/en/latest/esp32p4/esp32-p4-function-ev-board/user_guide.html#getting-started)|
| [wake_word_detection](./wake_word_detection)                       | WakeNet9       | [ESP32-Korvo](https://github.com/espressif/esp-skainet/blob/master/docs/en/hw-reference/esp32/user-guide-esp32-korvo-v1.1.md), [ESP32-S3-Korvo-1](https://github.com/espressif/esp-skainet/blob/master/docs/en/hw-reference/esp32s3/user-guide-korvo-1.md), [ESP-BOX](https://github.com/espressif/esp-box), [ESP-S3-Korvo-2](https://docs.espressif.com/projects/esp-adf/en/latest/get-started/user-guide-esp32-s3-korvo-2.html), [ESP32-S3-EYE](https://www.espressif.com/en/products/devkits/esp-s3-eye/overview), [ESP32-P4-Function-EV](https://docs.espressif.com/projects/esp-dev-kits/en/latest/esp32p4/esp32-p4-function-ev-board/user_guide.html#getting-started)|
| [chinese_tts](./chinese_tts)                                       | esp-tts-v1.7    | [ESP32-Korvo](https://github.com/espressif/esp-skainet/blob/master/docs/en/hw-reference/esp32/user-guide-esp32-korvo-v1.1.md), [ESP32-S3-Korvo-1](https://github.com/espressif/esp-skainet/blob/master/docs/en/hw-reference/esp32s3/user-guide-korvo-1.md), [ESP-BOX](https://github.com/espressif/esp-box), [ESP-S3-Korvo-2](https://docs.espressif.com/projects/esp-adf/en/latest/get-started/user-guide-esp32-s3-korvo-2.html), [ESP32-P4-Function-EV](https://docs.espressif.com/projects/esp-dev-kits/en/latest/esp32p4/esp32-p4-function-ev-board/user_guide.html#getting-started)|
| [usb_mic_recorder](./usb_mic_recorder)                                       |      | [ESP-BOX](https://github.com/espressif/esp-box), [ESP-S3-Korvo-2](https://docs.espressif.com/projects/esp-adf/en/latest/get-started/user-guide-esp32-s3-korvo-2.html)|

关于针对应用的具体配置，请参考每个示例中的 README.md。

## 软件准备

### ESP-Skainet

您可以使用以下命令克隆整个工程：

```
git clone https://github.com/espressif/esp-skainet.git 
```

### ESP-IDF

在本工程目前支持 [ESP-IDF v4.4](https://github.com/espressif/esp-idf/tree/release/v4.4) 和 [ESP-IDF v5.0](https://github.com/espressif/esp-idf/tree/release/v4.4) 版本。如果您之前已经搭建过 ESP-IDF 环境并且不想更改现有的变量，可以将 IDF_PATH 环境变量配置为新的 ESP-IDF 的路径 . 


  *注意:* 如果使用ESP-IDFv3.2及之前版本，建议参考esp-skainet v0.2.0

获取更多关于搭建 ESP-IDF 环境的细节，请参考 [Getting Started Guide for ESP-IDF release/v4.4 branch](https://docs.espressif.com/projects/esp-idf/en/release-v4.4/esp32/get-started/index.html)

# 示例

[examples](examples) 文件夹包含了使用 ESP-Skainet API 搭建的一些应用示例。

请从wake_word_detection示例开始：

1. 进入示例文件夹 `esp-skainet/examples/wake_word_detection:
```
cd esp-skainet/examples/wake_word_detection
```

2. 编译和烧写
```
idf.py flash monitor
```

3. 用户可以使用 idf.py menuconfig 命令添加或修改语音命令。

获取更多细节，请阅读示例中的 README.md

# 资源

* [在 GitHub 上参考问题](https://github.com/espressif/esp-skainet/issues) 如果你发现了一个 bug 或者提出一个 feature，请在开启新问题前参考已经存在的问题。

* 如果你对开发 ESP-Skainet 有兴趣, 清参考 [Contributions Guide](https://esp-idf.readthedocs.io/en/latest/contribute/index.html).
