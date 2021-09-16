# 垃圾分类例程 [[English]](./README.md)

（参考在上一级的 `examples` 目录下的 [README.md](../README.md) 文件来获取更多信息。）

在这个示例中，我们配置了 4 个命令词 ID：

* 湿垃圾
* 干垃圾
* 可回收垃圾
* 有害垃圾

我们已经通过 `menuconfig` 配置了 45 类常见的垃圾名称。

## 如何使用例程

### Hardware Required
### 硬件需求

- 这个示例能够在 ESP32-LyraT-Mini 开发板/ESP32-Korvo 开发板/ESP32-S3-Korvo 开发板上运行，关于开发板更多的信息，请参考 [ESP32-LyraT-Mini Getting Started Guide](https://docs.espressif.com/projects/esp-adf/en/latest/get-started/get-started-esp32-lyrat-mini.html) 或者 [ESP32-Korvo Getting Started Guide](https://github.com/espressif/esp-skainet/blob/master/docs/en/hw-reference/esp32/user-guide-esp32-korvo-v1.1.md).

- 一只音箱

### 配置工程

* 根据使用的开发板模组选择对应的 `sdkconfig`
  
  - 比如， 使用 ESP32， 运行 `cp sdkconfig_esp32.defaults sdkconfig`

  - 比如， 使用 ESP32-S3， 运行 `cp sdkconfig_esp32s3r8_8+4.defaults sdkconfig`

* 进入 `idf.py menuconfig`

* 通过 `Serial Flasher Options`设置串口信息

* 通过`ESP Speech Recognition`添加或更改垃圾名称

### 编译和烧写

编译并烧写，然后运行终端监控查看打印：

```
idf.py flash monitor
```

(退出窗口，请键入 ``Ctrl-]``.)

参考 [Getting Started Guide](https://docs.espressif.com/projects/esp-idf/en/stable/get-started-cmake/index.html) 来获取更多使用 ESP-IDF 编译项目的细节.

## 例程输出

以下是例程的上电打印输出:

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
I (268) MN: ---------------------------------------------------------

------------detect start------------

```

然后，说“ “Hi Lexin" ([Ləsɪ:n]) ” 来唤醒开发板，唤醒后会打印：

```
wakeword DETECTED.
-----------LISTENING-----------
```
同时，板子上的 LED 灯会亮起，直到进入下一次等待唤醒状态。

然后，板子会进入侦听状态，等到用户说出垃圾的名称。

比如，用户说出：“卫生纸”，

```
-----------LISTENING-----------
phrase ID: 0, prob: 0.866630
Commands ID: 0
干垃圾（Residual Waste）
-----------awaits to be waken up-----------

```

喇叭会播放对应的垃圾分类种类。