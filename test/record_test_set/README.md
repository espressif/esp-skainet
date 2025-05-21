
# WakeNet Test Set Recording Guide

## Overview
This document describes how to:
1. Synthesize test audio with varying SNR levels  
2. Play the audio through speakers  
3. Record audio using the ESP32 audio board  

The process involves two main components:  
- `create_test_set.py`: Synthesizes test audio and controls the test process  
- `sdcard_recorder`: ESP32 application that records audio to SD card based on serial commands  

**Supported hardware**:  
- [ESP32-S3-Korvo-1](https://github.com/espressif/esp-skainet/blob/master/docs/en/hw-reference/esp32s3/user-guide-korvo-1.md)  
- [ESP32-S3-Korvo-2](https://docs.espressif.com/projects/esp-adf/en/latest/get-started/user-guide-esp32-s3-korvo-2.html)  

---

## 1. Calibration Procedure

### 1.1 Noise Speaker Calibration
**Setup**:  
- Position two speakers according to your test scenario requirements  
- One speaker will play background noise  
- The other will play wake words  

**Calibration Steps**:  
1. Play the [pink noise sample](./data/noise_set/pink/Pink.wav) through the noise speaker  
2. Using a decibel meter at the DUT (Device Under Test) position:  
   - Adjust the speaker volume until the meter reads **55 dBA**  
   - Maintain this volume level for all subsequent tests  

### 1.2 DUT Playback Volume Calibration
**Setup**:  
Connect your ESP32 board to the test computer via USB and make sure the speaker is connected to the DUT's speaker output.

**Calibration Steps**:  
1. Play the [music sample](./data/music) through the DUT  
2. Using a decibel meter at the device's microphone position:  
   - Adjust the DUT's playback volume until the meter reads **70 dBA**  
   - Maintain this volume level for all playback tests  

---

## 2. Performance Testing

### 2.1 Configuration Setup
Modify `config.yml` according to your test requirements:

```yaml
clean_set:
  paths:
    - "./data/wake_words/hilexin"
  normalization: true
  target_dB: -36
music_set:
  paths:
    - "./data/music"
  normalization: true
  target_dB: -10
noise_set:
  paths:
    - "./data/noise_set/silence"
    - "./data/noise_set/pink"
    - "./data/noise_set/pub"
    - "./data/noise_set/news"
    - "./data/noise_set/cooking"
    - "./data/noise_set/washing"
  normalization: true
  target_dB: -36
output_set:
  path: "./data/output_set"
  overwrite: false
  noise_snr:
    - snr_dB: 5
      clean_gain_dB: 0
    - snr_dB: 0
      clean_gain_dB: 5
  playback_snr:
    - snr_dB: -10
      clean_gain_dB: 5
    - snr_dB: -15
      clean_gain_dB: 0

player:
  play_output: true


```

**Notes**:  
- The example configuration will generate 14 test cases (1 wake words × 6 noise types × 2 SNR levels + 2  playback SNR levels)  
- Only modify `target_dB` if you understand its impact on audio normalization  
- For playback tests, copy music files to `/sdcard/music` on your ESP32 device  
- **Supported audio format**: 16kHz, mono channel  

---

### 2.2 Flash the Recorder Application

```bash
# Navigate to the recorder directory
cd sdcard_recorder

# Configure the target board
idf.py menuconfig

#Build and flash
idf.py flash monitor
```

---

### 2.3 Run the Test

1. Connect your ESP32 board to the test computer via USB  
2. Verify the system detects the board's serial port  
3. Install required Python packages:  
   ```bash
   pip install -r requirement.txt
   ```
4. Execute the test script:  
   ```bash
   python create_test_set.py config.yml
   ```

**Test Process**:  
The script will automatically:  
1. Generate all test audio combinations  
2. Play each audio scenario  
3. Trigger the board to record responses  
4. Save recordings to the SD card  




# How to record MultiNet test set

Similar to recording WakeNet test set, `create_mn_test_set.py` is used to synthesize audio, play the audio, and record audio data for MultiNet test set.

## 1. configuration
The user can synthesize test set with different SNR by modifying the `config_mn.yml` file. The template of `config_mn.yml` is as follows:

```yml
clean_set:
  paths:
    - "./data/wake_words/hilexin"
  normalization: true
  target_dB: -36
music_set:
  paths:
    - "./data/music_set"
  normalization: true
  target_dB: -20
noise_set:
  paths:
    - "./data/noise_set/silence"
    - "./data/noise_set/pink"
    - "./data/noise_set/pub"
    - "./data/noise_set/news"
    - "./data/noise_set/cooking"
    - "./data/noise_set/washing"
  normalization: true
  target_dB: -36
output_set:
  path: "./data/output_set"
  overwrite: false
  noise_snr:
    - snr_dB: 10
      clean_gain_dB: 0
    - snr_dB: 5
      clean_gain_dB: 0
    - snr_dB: 0
      clean_gain_dB: 5
  playback_snr:
    - snr_dB: -10
      clean_gain_dB: 0
    - snr_dB: 0
      clean_gain_dB: 10

player:
  play_output: true

```
The above configuration will generate 3(the number of noise set) * 3(the number of SNR) = 9 test files.
Do not modify target_db unless understand its purpose.

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

