#!/bin/bash
port="/dev/ttyUSB1"
baud=$((2000000))
file="$1"

if [ -z "$1" ]; then
    echo "Flashes a wave file to the partition for the speech recognition thing to parse it."
    echo "Usage: $0 file.wav [/dev/ttyUSB0 [115200]]"
    exit 0
fi

if [ -n "$2" ]; then
    port="$2"
fi
if [ -n "$3" ]; then
    baud="$3"
fi

python ${IDF_PATH}/components/esptool_py/esptool/esptool.py \
        --port $port --baud $baud --before default_reset --after hard_reset write_flash \
        --flash_mode dio --flash_freq 40m --flash_size detect 0x410000 "$file"

