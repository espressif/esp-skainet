# wake_word_detection example [[中文]](./README_cn.md)

This example is used to test wake word detection.The data processing flow is as follows:
1. Audio data from microphone will be processed by speech enhancement module  
2. The processed data is inputed into wake word engine
3. If wake word is detected, a string like "hilexin DETECTED" will be printed. 
 
## How to use this example

### Hardware Required

- This example can run on ESP32-LyraT-Mini board and an external speaker connected to the board. For more information about ESP32-LyraT-Mini, please see [ESP32-LyraT-Mini Getting Started Guide](https://docs.espressif.com/projects/esp-adf/en/latest/get-started/get-started-esp32-lyrat-mini.html) or [ESP32-LyraT V4.3 Getting Started Guide](https://docs.espressif.com/projects/esp-adf/en/latest/get-started/get-started-esp32-lyrat.html)

### Configure the project

* Go to `make menuconfig`.

* Set the serial port number under `Serial Flasher Options`.

### Build and Flash

Build the project and flash it to the board, then run the monitor tool to view the output via serial port:

```
make -j4 flash monitor
```

(To exit the serial monitor, type ``Ctrl-]``.)

See the [Getting Started Guide](https://docs.espressif.com/projects/esp-idf/en/stable/get-started-cmake/index.html) for full steps to configure and use ESP-IDF to build projects.

## Example Output

There is the console output for this example:

```
Wakenet5x3_v2_hilexin_5_0.97_0.90
```

Indicates that the WakeNet version and wake word.

If the wake word is detected, the wake work is printed as follows:

```
1.00s: hilexin DETECTED.
```

