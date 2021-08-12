## 烧写 
/home/sunxiangyu/.espressif/python_env/idf4.1_py3.8_env/bin/python ../../../../esp/esp-idf-v4_1/esp-idf/components/esptool_py/esptool/esptool.py -p (PORT) -b 460800 --before default_reset --after hard_reset --chip esp32  write_flash --flash_mode dio --flash_size detect --flash_freq 80m 0x1000 build/bootloader/bootloader.bin 0x8000 build/partition_table/partition-table.bin 0x10000 build/recorder.bin
or run 'idf.py -p (PORT) flash'

## 使用说明

1. 上电，插入sdcard,按RST键开始一直录音．
2. 录音文件格式为pcm(16bit, 16000Hz, 2ch),录音文件每512MB保存一个文件，每次RST不会覆盖之前录音
3. 如果录音停止会有灯闪烁
