# Default firmware

## overview

esp32_korvo_v1_1_fw_v0_1_20200323.bin is the default firmware of ESP32-Korvo V1.1.

default_firmware_ESP32-S3-Korvo-1.bin is the default firmware of ESP32-S3-Korvo-1.

## Usgae

### Setting Up ESP-IDF

See https://idf.espressif.com/ for links to detailed instructions on how to set up the ESP-IDF depending on chip you use.

### Flash bin

```
python $IDF_PATH/components/esptool_py/esptool/esptool.py --chip esp32 --port /dev/ttyUSB0 --baud 115200 --before default_reset --after hard_reset write_flash -z --flash_mode dio --flash_freq 80m --flash_size detect 0x0000 esp32_korvo_v1_1_fw_v0_1_20200323.bin`
```

```
python $IDF_PATH/components/esptool_py/esptool/esptool.py --chip esp32s3 --port /dev/ttyUSB0 --baud 115200 --before default_reset --after hard_reset write_flash -z --flash_mode dio --flash_freq 80m --flash_size detect 0x0000 default_firmware_ESP32-S3-Korvo-1.bin`
```

For Windows users ， [Espressif Flash Download Tools](https://www.espressif.com/zh-hans/support/download/other-tools) also can be used to flash bin。

