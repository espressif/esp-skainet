This program is used to test recorded set and generate a test report.

## How to start

###  1. Create RAR test set
The test set is a csv file, which contains the file path and the number of wake words, formatted as follows:
```
filename,required,total
/sdcard/hilexin_0dB_silence.wav,285,300
/sdcard/hilexin_0dB_pub_-10dB.wav,270,300
/sdcard/hilexin_0dB_pink_-10dB.wav,270,300
/sdcard/hilexin_0dB_pub_-5dB.wav,270,300
/sdcard/hilexin_0dB_pink_-5dB.wav,270,300
/sdcard/hilexin_5dB_pub_5dB.wav,260,300
/sdcard/hilexin_5dB_pink_5dB.wav,260,300
```

The filename of this csv file should be "{wn_name}.csv". `wn_name` is the name of the wake word model you loaded in `menuconfig`. If you want to use other filename, please modify the [wakenet_main.c](./main/wakenet_main.c). 

`required` means the number of wake words required to pass the test.   
`total` means the total number of wake words in the test set.   

### 2. Download FAR test set

FAR test set is general for all wake word models. Please download it from the following link
download [far_48h.csv](https://github.com/espressif/esp-adf/blob/master/examples/speech_recognition/wake_word_detection/far_48h.csv)

This FAR test set consists of 48 hours of three-channel audio data, including:  
- 22 hours of Chinese speech  
- 20 hours of English speech  
- 6 hours of music  

Please copy the data to the SD card before you start the test.

### 2. build and flash 
Insert the SD card and flash the program.

```
idf.py flash monitor
```

### 3. test

Type `rar` in the terminal to load `{wn_name}.csv` and start the RAR test.
Type `far` in the terminal to load `far_48h.csv` and start the FAR test.

