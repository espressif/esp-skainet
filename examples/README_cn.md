# 示例 [[English]](./README.md)

这个文件夹包含了 ESP-Skainet 中的所有示例。这些示例的是为了演示 ESP-Skainet 中模型和算法的使用方法，并提供在项目开发过程中可以复用的源代码。  
推荐使用ESP32-S3芯片以及相关开发版．新的算法和模型将优先部署在ESP32-S3上．


# 示例设计


The following examples are currently available:

|                          Example Name                               |   Latest Models   |  Supported Board   |
| :------------------------------------------------------------------ | :---------------: | :-------------- |
| [cn_speech_commands_recognition](./cn_speech_commands_recognition) | Multinet6      | [ESP32-Korvo](https://github.com/espressif/esp-skainet/blob/master/docs/en/hw-reference/esp32/user-guide-esp32-korvo-v1.1.md), [ESP32-S3-Korvo-1](https://github.com/espressif/esp-skainet/blob/master/docs/en/hw-reference/esp32s3/user-guide-korvo-1.md), [ESP-BOX](https://github.com/espressif/esp-box), [ESP-S3-Korvo-2](https://docs.espressif.com/projects/esp-adf/en/latest/get-started/user-guide-esp32-s3-korvo-2.html), [ESP32-S3-EYE](https://www.espressif.com/en/products/devkits/esp-s3-eye/overview)|
| [en_speech_commands_recognition](./en_speech_commands_recognition) | Multinet6      | [ESP32-S3-Korvo-1](https://github.com/espressif/esp-skainet/blob/master/docs/en/hw-reference/esp32s3/user-guide-korvo-1.md), [ESP-BOX](https://github.com/espressif/esp-box), [ESP-S3-Korvo-2](https://docs.espressif.com/projects/esp-adf/en/latest/get-started/user-guide-esp32-s3-korvo-2.html), [ESP32-S3-EYE](https://www.espressif.com/en/products/devkits/esp-s3-eye/overview)|
| [wake_word_detection](./wake_word_detection)                       | Wakenet9      | [ESP32-Korvo](https://github.com/espressif/esp-skainet/blob/master/docs/en/hw-reference/esp32/user-guide-esp32-korvo-v1.1.md), [ESP32-S3-Korvo-1](https://github.com/espressif/esp-skainet/blob/master/docs/en/hw-reference/esp32s3/user-guide-korvo-1.md), [ESP-BOX](https://github.com/espressif/esp-box), [ESP-S3-Korvo-2](https://docs.espressif.com/projects/esp-adf/en/latest/get-started/user-guide-esp32-s3-korvo-2.html), [ESP32-S3-EYE](https://www.espressif.com/en/products/devkits/esp-s3-eye/overview)|
| [chinese_tts](./chinese_tts)                                       | esp-tts-v1.7    | [ESP32-Korvo](https://github.com/espressif/esp-skainet/blob/master/docs/en/hw-reference/esp32/user-guide-esp32-korvo-v1.1.md), [ESP32-S3-Korvo-1](https://github.com/espressif/esp-skainet/blob/master/docs/en/hw-reference/esp32s3/user-guide-korvo-1.md), [ESP-BOX](https://github.com/espressif/esp-box), [ESP-S3-Korvo-2](https://docs.espressif.com/projects/esp-adf/en/latest/get-started/user-guide-esp32-s3-korvo-2.html)|
| [usb_mic_recorder](./usb_mic_recorder)                                       |      | [ESP-BOX](https://github.com/espressif/esp-box), [ESP-S3-Korvo-2](https://docs.espressif.com/projects/esp-adf/en/latest/get-started/user-guide-esp32-s3-korvo-2.html)|

# 贡献示例

如果您有任何要与我们分享的示例，或者您认为我们可能感兴趣，请参考 [Contributions Guide](https://esp-idf.readthedocs.io/en/latest/contribute/index.html) 并且联系我们。


