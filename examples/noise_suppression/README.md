# noise_suppression example [[中文]](./README_cn.md)

(See the [README.md](../README.md) file in the upper level 'examples' directory for more information about examples.)

This example takes the audio data collected by the microphone through noise suppression to obtain 16KHz, 16bit, mono audio data, and immediately outputs the noise-reduced data. The user can select whether the noise suppression is turned on by pressing the "Mode" button on the LyraT_Mini development board.
 
## How to use this example

### Hardware Required

- This example can run on ESP32-LyraT-Mini board and an external speaker connected to the board. For more information about ESP32-LyraT-Mini, please see [ESP32-LyraT-Mini Getting Started Guide](https://docs.espressif.com/projects/esp-adf/en/latest/get-started/get-started-esp32-lyrat-mini.html).

- headphone

### Configure the project

* Go to `make menuconfig`.

* Set the serial port number under `Serial Flasher Options`.

* Add or modify the garbage name, such as “wei sheng zhi” (toilet paper).

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
NS STATE: 1
```

Indicates that the initial state is the state in which noise suppression is turned on, you can hear the noise after the noise reduction process by headphone.

Press the "Mode" button and the following output will be displayed:

```
NS STATE: 0
```

Indicates that the current state is the state of noise suppression off. Pressing the "Mode" button again will enter the mode of noise suppression on.