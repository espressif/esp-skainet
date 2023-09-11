# How to record WakeNet test set
`create_test_set.py` is used to synthesize audio with different SNR, play the audio, and use the serial port to record audio data by the board.

`sdcard_recorder` app records and saves data into SD card by receiving the serial port signal from `create_test_set.py`. This project supports [ESP32-Korvo](https://github.com/espressif/esp-skainet/blob/master/docs/en/hw-reference/esp32/user-guide-esp32-korvo-v1.1.md), [ESP32-S3-Korvo-1](https://github.com/espressif/esp-skainet/blob/master/docs/en/hw-reference/esp32s3/user-guide-korvo-1.md),[ESP-S3-Korvo-2](https://docs.espressif.com/projects/esp-adf/en/latest/get-started/user-guide-esp32-s3-korvo-2.html).

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

## 3. synthesize, play and record

```
pip install -r requirement.txt

python create_test_set.py config.yml
```


# How to record MultiNet test set
Similar to recording WakeNet test set, `create_mn_test_set.py` is used to synthesize audio, play the audio, and record audio data for MultiNet test set.

## 1. configuration
The user can synthesize test set with different SNR by modifying the `config_mn.yml` file. The template of `config_mn.yml` is as follows:

```yml
clean_set:
  wake_words_paths:
    - "data/hilexin"       # the directory contains wake word recordings
  commands_paths:
    - "data/CN-TEST-S"     # the directory contains command recordings
  filelists_paths:
    - "data/cn_test.json"  # a JSON file contains the recording orders
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
  path: "data/test"        # the directory contains the output recordings
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
The above configuration will generate 3(the number of noise set) * 3(the number of SNR) = 9 test files.

## 2. JSON file
The JSON file referred in `config_mn.yml` needs to be generated beforehand, which should use the following format:
```json
[
  {
    'wake_word_fname': 'K000000000000-16316211672.wav',
    'size': 27648,
    'wake_word_tailing_silence_ms': 100,
    'commands': [
      {'command_fname': 'brian-3.wav', 'size': 95744},
      {'command_fname': 'Omar-3_1.wav', 'size': 95744},
      {'command_fname': 'Jakob-8_3.wav', 'size': 95744},
      {'command_fname': 'kirill-12_1.wav', 'size': 95744}],
    'between_command_length_ms': 5000
  },
  {
    'wake_word_fname': 'K000000000000-1631851371-193-83532.wav',
    'size': 26624,
    'wake_word_tailing_silence_ms': 100,
    'commands': [
      {'command_fname': 'Darian-21_3.wav', 'size': 95744},
      {'command_fname': 'kirill-39_2.wav', 'size': 95744},
      {'command_fname': 'jeroen-25.wav', 'size': 95744},
      {'command_fname': 'Jakob-22_1.wav', 'size': 95744}],
    'between_command_length_ms': 5000
  },
  ...
]
```
- `'wake_word_fname'` is the filename of a wake word recording
- `'size'` is the number of samples of a recording
- `'wake_word_tailing_silence'` is the length of silence (ms) between the wake word and the first command.
- `'command_fname'` is the filename of a command recording
- `'between_command_length_ms'` is the length of silence (ms) between command recordings, **if the command recording you are using does not have long silent tail, make sure that there are at least 2 to 3 seconds of silence between two commands**.

The following command provides an example of how to create the JSON file, you might want to modify the `wake_word_tailing_silence` and `between_command_length_ms` in it for your own test data.

```sh
python create_mn_test_json.py \
    --wake-word-dir data/hiesp/ \
    --command-dir data/espen \
    --out-file en_mn_test.json
```
## 3. specifics

- the recorded audio follows a format of: one wake word recording followed by a few command recordings.
- there should be at least 2 to 3 seconds of silence between two consecutive commands.
- the reason of using an standalone JSON file to store the file order is for reproducibility and as a way to store command ID sequence for evaluation.

