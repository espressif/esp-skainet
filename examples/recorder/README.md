# noise_suppression example [[中文]](./README_cn.md)

(See the [README.md](../README.md) file in the upper level 'examples' directory for more information about examples.)

This example takes the audio data collected by the single microphone through noise suppression to obtain 16KHz, 16bit, mono audio data, and immediately outputs the noise-reduced data　into SD card. The user can select whether the noise suppression is turned on by pressing the "Mode" button on the LyraT_Mini development board or ESP32-Korvo V1.1 board.


## How to use this example

### Hardware Required

- This example can run on ESP32-LyraT-Mini board and ESP32-Korvo board. For more information about ESP32-LyraT-Mini, please see [ESP32-LyraT-Mini Getting Started Guide](https://docs.espressif.com/projects/esp-adf/en/latest/get-started/get-started-esp32-lyrat-mini.html or [ESP32-Korvo Getting Started Guide](https://github.com/espressif/esp-skainet/blob/master/docs/en/hw-reference/esp32/user-guide-esp32-korvo-v1.1.md) for more information.

- SD card

### Configure the project

* Go to `idf.py menuconfig`.

* Set the serial port number under `Serial Flasher Options`.

### Build and Flash

Build the project and flash it to the board, then run the monitor tool to view the output via serial port:

```
idf.py flash monitor
```

(To exit the serial monitor, type ``Ctrl-]``.)

See the [Getting Started Guide](https://docs.espressif.com/projects/esp-idf/en/stable/get-started-cmake/index.html) for full steps to configure and use ESP-IDF to build projects.

## Example Output

There is the console output for this example:

```
Start to record: 
NS STATE: 1
record:1 s
record:2 s
record:3 s
record:4 s
record:5 s
...
```

Press the `Mode` button to enable/disable noise suppression  
Press the `Rec` button to stop recording. **Note: if do not press `Rec` button before pull out SD card, the wav file can not be encoded correctly.**  
Press the `RST` button to restart to record  