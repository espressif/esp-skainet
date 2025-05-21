# WakeNet Test Report Generation Guide

This program automates the testing of recorded audio sets and generates comprehensive test reports for wake word detection performance.

## Test Preparation

### 1. Create RAR Test Set (Wake Word Detection Test)

**File Format Requirements**:
- CSV file containing test cases and pass criteria
- Must be named `{wn_name}.csv` (where `wn_name` matches your wake word model name in `menuconfig`)
- Store in the root directory of the SD card

**File Structure Example**:
```csv
filename,required,total
/sdcard/hilexin_0dB_silence.wav,285,300
/sdcard/hilexin_0dB_pub_-10dB.wav,270,300
/sdcard/hilexin_0dB_pink_-10dB.wav,270,300
/sdcard/hilexin_0dB_pub_-5dB.wav,270,300
/sdcard/hilexin_0dB_pink_-5dB.wav,270,300
/sdcard/hilexin_5dB_pub_5dB.wav,260,300
/sdcard/hilexin_5dB_pink_5dB.wav,260,300
```

**Field Definitions**:
| Field    | Description |
|----------|-------------|
| filename | Full path to test audio file on SD card |
| required | Minimum number of successful detections needed to pass (typically 85%-95% of total) |
| total    | Total number of wake word instances in the audio file |

**Customization Note**:  
To use a different filename, modify the source file:  
[wakenet_main.c](./main/wakenet_main.c)

---

### 2. Prepare FAR Test Set (False Alarm Test)

**General Requirements**:
- Standardized test set for all wake word models
- 48 hours of continuous audio data
- Must be named `far_48h.csv`
- Store in the root directory of the SD card

**Audio Content Composition**:
| Content Type | Duration | Description |
|--------------|----------|-------------|
| Chinese Speech | 22 hours | Various accents and speaking styles |
| English Speech | 22 hours | Multiple dialects and pronunciations |
| Music         | 4 hours  | Different genres and recording qualities |

**File Preparation**:
1. Download the FAR test package
2. Extract contents to SD card root directory
3. Verify all files are properly transferred

---

## Setup and Execution

### 1. Hardware Preparation
1. Insert prepared SD card into the device
2. Ensure proper connection to host computer via USB

### 2. Build and Flash Firmware
```bash
# Build and flash the program
idf.py flash monitor

# For first-time setup:
idf.py set-target esp32s3  # Specify your target chip if different
```

### 3. Run Tests

**FAR Test Execution**:
1. In serial monitor, type command:  
   ```
   far
   ``` 
2. Program will:
   - Process the 48-hour test set
   - Monitor for false wake word detections
   - Calculate false alarm rate
   - Generate comprehensive report

**NOTE**: The FAR test automatically enables **debug mode** by default. In debug mode, the system will output **detection threshold**. Please adjust the threshold based on the test results before conducting the RAR test.


**RAR Test Execution**:
1. Open serial monitor after flashing
2. Type command:  
   ```
   rar
   ```
3. Program will:
   - Load `{wn_name}.csv`
   - Process each test file
   - Generate detection statistics
   - Output pass/fail results
---
