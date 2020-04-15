# 降噪例程 [[English]](./README.md)

（参考在上一级的 `examples` 目录下的 [README.md](../README.md) 文件来获取更多信息。）

在这个示例中,将单麦克风采集的声音经过降噪处理，得到 16KHz，16bit，单声道的音频数据，并且立即将降噪后的数据通过耳机孔输出。用户可以通过按下 LyraT_Mini 开发板或 ESP32-Korvo V1.1 开发板上的 "Mode" 按键来选择开启或关闭降噪。 该示例主要展示单麦克法降噪算法，麦克风真理语音增强请参考示例[mic_array_speech_enhancement](../mic_array_speech_enhancement).

## 如何使用例程

### Hardware Required
### 硬件需求

- 这个示例能够在 ESP32-LyraT-Mini 开发板或 ESP32-Korvo V1.1 开发板上运行，关于开发板更多的信息，请参考 [ESP32-LyraT-Mini Getting Started Guide](https://docs.espressif.com/projects/esp-adf/en/latest/get-started/get-started-esp32-lyrat-mini.html) 或者 [ESP32-Korvo Getting Started Guide](https://github.com/espressif/esp-skainet/blob/master/docs/en/hw-reference/esp32/user-guide-esp32-korvo-v1.1.md)
- 耳机

### 配置工程

* 进入 `idf.py menuconfig`

* 通过 `Serial Flasher Options`设置串口信息

### 编译和烧写

编译并烧写，然后运行终端监控查看打印：

```
idf.py flash monitor
```

(退出窗口，请键入 ``Ctrl-]``.)

参考 [Getting Started Guide](https://docs.espressif.com/projects/esp-idf/en/stable/get-started-cmake/index.html) 来获取更多使用 ESP-IDF 编译项目的细节.

## 例程输出

上电打印：

```
NS STATE: 1
```

表示初始状态为降噪打开的状态。当插上耳机时，就可以听到经过降噪处理后的声音。


```
NS STATE: 0
```

表示目前状态为降噪关闭的状态。再次按下 "Mode" 键则又会进入降噪打开的模式。
