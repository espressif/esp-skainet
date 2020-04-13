# 默认固件说明

## 固件信息

esp32_korvo_v1_1_fw_v0_1_20200323.bin 是 ESP32-Korvo V1.1 开发板的默认出厂固件，适用 80MHz DIO 模式 SPI，烧写地址为 0x0000。

## 烧写方法

在已配置 ESP-IDF 开发环境的电脑上，运行

`python $IDF_PATH/components/esptool_py/esptool/esptool.py --chip esp32 --port /dev/ttyUSB0 --baud 115200 --before default_reset --after hard_reset write_flash -z --flash_mode dio --flash_freq 80m --flash_size detect 0x0000 esp32_korvo_v1_1_fw_v0_1_20200323.bin`

即可完成烧写。注意串口号和固件路径需要根据实际情况填写。

在 Windows 上，用户也可使用 [Espressif 烧写工具](https://www.espressif.com/sites/default/files/tools/flash_download_tools_v3.6.8.zip) 进行烧写。