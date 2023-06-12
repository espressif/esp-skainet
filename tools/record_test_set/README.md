# How to record test set
`create_test_set.py` is used to synthesize audio with different SNR, play the audio, and use the serial port to control the recording of the board

sdcard_recorder records and save data into SD card by receiving the serial port signal from `create_test_set.py`. This project supports esp32-korvo, esp32-korvo-1, esp32-korvo-2.

## 1. configuration
The user can synthesize test set with different SNR by modifying the `config.yml` file. The template of `config.yml` is as follows:

```yml
clean_set:
  paths:
    - "./data/clean_set/hiesp"
    - "./data/clean_set/hilexin"
  normalization: True
  target_dB: -36
noise_set:
  paths:
    - "./data/noise_set/silence"
    - "./data/noise_set/pink"
    - "./data/noise_set/pub"
  normalization: True
  target_dB: -36
output_set:
  path: "./data/output_set"
  remove_old_files: true
  snr:
    - snr_dB: 10
      clean_gain_dB: 0
    - snr_dB: 5
      clean_gain_dB: 0
    - snr_dB: 0
      clean_gain_dB: 5
player:
  play_output: true
   
```
The above configuration will generate 2(the number of clean set)*3(the number of noise set)*3(the number of SNR)=18 test files.

## 2. build and flash sdcard_recorder

```
cd sdcard_recorder
idf.py menuconfig   // select board and change the target
idf.py flash monitor   // build and flash
```

## 2. synthesize, play and record

```
pip install -r requirement.txt

python create_test_set.py config.yml
```
