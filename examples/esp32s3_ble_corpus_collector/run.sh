input_dir="/media/sunxiangyu/444D-30A9/blufi-recorder-bin"
PORT="/dev/ttyUSB0"

/home/sunxiangyu/.espressif/python_env/idf4.4_py3.8_env/bin/python $IDF_PATH//components/esptool_py/esptool/esptool.py -p $PORT -b 460800 --before default_reset --after hard_reset --chip esp32s3  write_flash --flash_mode dio --flash_size detect --flash_freq 80m 0x0 $input_dir/bootloader.bin 0x8000 $input_dir/partition-table.bin 0x10000 $input_dir/esp32s3_ble_corpus_recorder.bin
