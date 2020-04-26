# mic_array_speech_enhancement example [[中文]](./README_cn.md)

(See the [README.md](../README.md) file in the upper level 'examples' directory for more information about examples.)

This example takes the multi-channel audio data collected by the microphone array through Espressif's mic-array speech enhancement (MASE) algorithm to obtain 16kHz 16bit mono audio data, and immediately outputs the enhanced audio data via your earphone. You can choose whether to turn on MASE by pressing the "Mode" button, and changing the states of the LED lights by pressing other buttons. In the meantime, the enhanced signal is sent to Espressif's local wake word detection (WakeNet) and local speech commands recognition (MultiNet), you can also control the states of LED lights via your voice.
 
## How to use this example

### Prepare hardware

* This example runs on ESP32-Korvo board. For more information, please refer to [ESP32-Korvo Getting Started Guide](https://github.com/espressif/esp-skainet/blob/master/docs/en/hw-reference/esp32/user-guide-esp32-korvo-v1.1.md).

* Earphone.

### Configure the project

* Go to `idf.py menuconfig`.

* Set the serial port number under `Serial flasher config`.

* Enter `Audio Media HAL` menu, select Korvo as `Audio hardware board`, and choose the desired `Mic-array type`.

### Build and Flash

Build the project and flash it to the board, then run the monitor tool to view the output via serial port:

```
idf.py flash monitor
```

(To exit the serial monitor, type ``Ctrl-]``.)

See the [Getting Started Guide](https://docs.espressif.com/projects/esp-idf/en/stable/get-started-cmake/index.html) for full steps to configure and use ESP-IDF to build projects.

### Use the buttons

There are six function buttons, i.e. `PLAY`, `SET`, `VOL-`, `VOL+`, `MODE` and `REC` on ESP32-Korvo. In this example, button `MODE` is used to control whether MASE algorithm is enabled, while the others are used to control the states of LED lights.

The initial console output of this example will be:

```
MASE STATE: 1
```

which indicates that the initial MASE state is ON. Press "Mode" button and the following output will be displayed:

```
MASE STATE: 0
```

indicating MASE is turned off. Pressing "Mode" button again will turn MASE back on, and so on.

The corresponding relationship between buttons and LED states is:

|Button|Response|
|:- |:- |
|PLAY|RGB LEDs glow white.|
|SET|RGB LEDs glow red.|
|VOL+|RGB LEDs glow yellow.|
|VOL-|RGB LEDs glow blue.|
|REC|RGB LEDs go out.|

### Use wake word detection and speech commands recognition

After powering up, you can activate the board with the default wake word “Hi, Le Xin”, which translates in English as “Hello, Espressif”. When the wake word is detected, the 12 on-board RGB LEDs glow white one by one, meaning that the board is waiting for a speech command.

This example supports speech commands to control LED states, the corresponding relationship is:

|Default Speech Command (in Chinese)|Meaning|Response|
|:- |:- |:- |
|Da Kai Bai Deng|Turn on the white light|RGB LEDs glow white.|
|Da Kai Hong Deng|Turn on the red light|RGB LEDs glow red.|
|Da Kai Huang Deng|Turn on the yellow light|RGB LEDs glow yellow.|
|Da Kai Lan Deng|Turn on the blue light|RGB LEDs glow blue.|
|Guan Bi Dian Deng|Turn off the light|RGB LEDs go out.|

## Example Output

### Audio stream

You can listen to the enhanced audio signal with your earphone. For a more straightforward illustration, you can record and view the signal before and after MASE with your audio analysis software. In this example, MASE operates in `WAKE_UP_ENHANCEMENT_MODE`, which means that it is specially designed to enhance the wake-up word (and following speech for local or cloud speech recognition, if there is any). A typical input/output comparison for  2-mic MASE is as follows, in which wake-up words are clearly highlighted:

* time domain

![MASE_td](MASE_td.png)

* frequency domain

![MASE_fd](MASE_fd.png)

### Improvement on speech recognition

MASE can significantly improve the performance of the following speech recognition stage, including local wake word detection (WakeNet), local speech commands recognition (MultiNet), and cloud service. Test results show that in 5dB SNR environment, 2-mic and 3-mic MASE can improve over **15%** and **20%** WakeNet performance and over **10%** and **15%** MultiNet performance comparing to 1-mic system.