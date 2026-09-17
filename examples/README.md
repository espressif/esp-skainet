# Examples [[中文]](./README_cn.md)



This directory contains a range of examples in ESP-Skainet projects. These examples exist to demonstrate the functionality of ESP-Skainet and to provide the source codes that can be copied and adapted to during the development of your projects.
The ESP32-S3 chip and boards are recommended. New algorithms and models will be deployed on the ESP32-S3 chip first.


# Example Layout



The following examples are currently available:

|                          Example Name                               |   Latest Models   |  Supported Board   |
| :------------------------------------------------------------------ | :---------------: | :-------------- |
| [cn_speech_commands_recognition](./cn_speech_commands_recognition) | Multinet6      | [ESP32-Korvo](https://github.com/espressif/esp-skainet/blob/master/docs/en/hw-reference/esp32/user-guide-esp32-korvo-v1.1.md), [ESP32-S3-Korvo-1](https://github.com/espressif/esp-skainet/blob/master/docs/en/hw-reference/esp32s3/user-guide-korvo-1.md), [ESP-BOX](https://github.com/espressif/esp-box), [ESP-S3-Korvo-2](https://docs.espressif.com/projects/esp-adf/en/latest/get-started/user-guide-esp32-s3-korvo-2.html), [ESP32-S3-EYE](https://www.espressif.com/en/products/devkits/esp-s3-eye/overview)|
| [en_speech_commands_recognition](./en_speech_commands_recognition) | Multinet6      | [ESP32-S3-Korvo-1](https://github.com/espressif/esp-skainet/blob/master/docs/en/hw-reference/esp32s3/user-guide-korvo-1.md), [ESP-BOX](https://github.com/espressif/esp-box), [ESP-S3-Korvo-2](https://docs.espressif.com/projects/esp-adf/en/latest/get-started/user-guide-esp32-s3-korvo-2.html), [ESP32-S3-EYE](https://www.espressif.com/en/products/devkits/esp-s3-eye/overview)|
| [wake_word_detection](./wake_word_detection)                       | Wakenet9       | [ESP32-Korvo](https://github.com/espressif/esp-skainet/blob/master/docs/en/hw-reference/esp32/user-guide-esp32-korvo-v1.1.md), [ESP32-S3-Korvo-1](https://github.com/espressif/esp-skainet/blob/master/docs/en/hw-reference/esp32s3/user-guide-korvo-1.md), [ESP-BOX](https://github.com/espressif/esp-box), [ESP-S3-Korvo-2](https://docs.espressif.com/projects/esp-adf/en/latest/get-started/user-guide-esp32-s3-korvo-2.html), [ESP32-S3-EYE](https://www.espressif.com/en/products/devkits/esp-s3-eye/overview)|
| [chinese_tts](./chinese_tts)                                       | esp-tts-v1.7    | [ESP32-Korvo](https://github.com/espressif/esp-skainet/blob/master/docs/en/hw-reference/esp32/user-guide-esp32-korvo-v1.1.md), [ESP32-S3-Korvo-1](https://github.com/espressif/esp-skainet/blob/master/docs/en/hw-reference/esp32s3/user-guide-korvo-1.md), [ESP-BOX](https://github.com/espressif/esp-box), [ESP-S3-Korvo-2](https://docs.espressif.com/projects/esp-adf/en/latest/get-started/user-guide-esp32-s3-korvo-2.html)|


# Target Tests

Every example ships a `pytest_*.py` case that flashes the example on a board and checks that it
starts up and keeps running without panicking. These cases run in the `target_test` stage of CI,
and you can run them locally on an ESP32-S3-Korvo-2 or an ESP32-P4-Function-EV board:

```bash
. ${IDF_PATH}/export.sh
pip install idf_build_apps
pip install -r tools/ci/requirements.txt -c ${IDF_TOOLS_PATH}/espidf.constraints.v5.5.txt

# Build every example for the board the tests expect
python -m idf_build_apps build -p examples --recursive \
    --manifest-file examples/.build-test-rules.yml \
    -t esp32s3 --build-dir "5.5/build_@t_@w" --config-rules "=default" \
    --override-sdkconfig-files tools/ci/sdkconfig.ci.esp32s3

# Flash and test them one after another
pytest examples --target esp32s3 --env korvo-2 --config default
```

Pass a single example directory to `pytest` to run just that one, and `--port` to pick a specific
serial port. The constraint file keeps `esptool` at the version the ESP-IDF release expects,
otherwise installing the pytest packages breaks `idf.py`.

# Contributing Examples
If you have any examples you want to share with us or you think may interest us, please check the [Contributions Guide](https://esp-idf.readthedocs.io/en/latest/contribute/index.html) and get in touch with us.
