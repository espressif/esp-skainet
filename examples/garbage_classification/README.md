# Garbage Sorting example [[中文]](./README_cn.md)

(See the [README.md](../README.md) file in the upper level 'examples' directory for more information about examples.)

In this example, we have configured four command IDs to represent:

* 湿垃圾 (Household Food Waste)
* 干垃圾 (Residual Waste)
* 可回收垃圾 (Recyclable Waste)
* 有害垃圾 (Hazardous Waste)

And we have configured 45 kinds of garbages with their corresponding command IDs in `menuconfig`.
 
## How to use this example

### Hardware Required

- This example can run on ESP32-LyraT-Mini board and an external speaker connected to the board. For more information about ESP32-LyraT-Mini, please see [ESP32-LyraT-Mini Getting Started Guide](https://docs.espressif.com/projects/esp-adf/en/latest/get-started/get-started-esp32-lyrat-mini.html).

- A Speaker

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
Quantized wakeNet5: wakeNet5_v1_hilexin_5_0.95_0.90, mode:0
Quantized MN1
I (198) MN: ---------------------SPEECH COMMANDS---------------------
I (197) MN: Command ID0, phrase 0: wei sheng zhi
I (197) MN: Command ID0, phrase 1: shi zhi jin
I (207) MN: Command ID0, phrase 2: shi pin dai
I (207) MN: Command ID0, phrase 3: can jin zhi
I (217) MN: Command ID0, phrase 4: niao bu shi
I (217) MN: Command ID0, phrase 5: mao sha
I (227) MN: Command ID0, phrase 6: mao fa
I (227) MN: Command ID0, phrase 7: yi ci xing can ju
I (237) MN: Command ID0, phrase 8: jiu mao jin
I (237) MN: Command ID0, phrase 9: tao ci zhi pin
I (247) MN: Command ID0, phrase 10: bei ke
I (247) MN: Command ID0, phrase 11: fa jiao
I (257) MN: Command ID0, phrase 12: sao ba
I (257) MN: Command ID0, phrase 13: da huo ji
I (267) MN: Command ID1, phrase 14: gua zi ke
I (267) MN: Command ID1, phrase 15: cha ye zha
I (277) MN: Command ID1, phrase 16: ji rou
I (277) MN: Command ID1, phrase 17: cai ye
I (287) MN: Command ID1, phrase 18: gua guo pi
I (287) MN: Command ID1, phrase 19: sheng fan sheng cai
I (297) MN: Command ID1, phrase 20: zhong yao yao zha
I (297) MN: Command ID1, phrase 21: bing gan
I (307) MN: Command ID1, phrase 22: yu mi
I (307) MN: Command ID1, phrase 23: ji gu tou
I (317) MN: Command ID1, phrase 24: xi hong shi
I (317) MN: Command ID1, phrase 25: hua sheng ke
I (327) MN: Command ID2, phrase 26: niu kou dian chi
I (327) MN: Command ID2, phrase 27: guo qi yao pin
I (337) MN: Command ID2, phrase 28: lao shu yao
I (337) MN: Command ID2, phrase 29: fei yao pin
I (347) MN: Command ID2, phrase 30: fei you qi
I (347) MN: Command ID2, phrase 31: you qi tong
I (357) MN: Command ID2, phrase 32: ying guang deng
I (367) MN: Command ID2, phrase 33: sha chong ji
I (367) MN: Command ID3, phrase 34: su liao ping
I (377) MN: Command ID3, phrase 35: yi la guan
I (377) MN: Command ID3, phrase 36: kuai di zhi xiang
I (387) MN: Command ID3, phrase 37: jiu bao zhi
I (387) MN: Command ID3, phrase 38: guan tou he
I (397) MN: Command ID3, phrase 39: ying su liao
I (397) MN: Command ID3, phrase 40: bao zhuang zhi
I (407) MN: Command ID3, phrase 41: jiu tie guo
I (407) MN: Command ID3, phrase 42: lan qiu
I (417) MN: Command ID3, phrase 43: bo li hu
I (417) MN: Command ID3, phrase 44: jiu wan ju
I (268) MN: ---------------------------------------------------------

chunk_num = 200
-----------awaits to be waken up-----------

```
Then, say “Hi Lexin" ([Ləsɪ:n]) to wake up the board, which then wakes up and prints the following log:

```
hilexin DETECTED.
-----------LISTENING-----------
```
During the meantime, the led on the board will remian on until the board enters the next Waiting-for-Wakeup Status.

Then, the board enters the Listening status, waiting for garbage name.  

Now, you can give one speech command, for example, “卫生纸” (toilet paper),

```
-----------LISTENING-----------
phrase ID: 0, prob: 0.866630
Commands ID: 0
干垃圾（Residual Waste）
--------------END--------------

```

The speaker will play the type of the garbage identified.


