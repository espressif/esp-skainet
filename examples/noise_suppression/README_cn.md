# 降噪例程 [[English]](./README.md)

（参考在上一级的 `examples` 目录下的 [README.md](../README.md) 文件来获取更多信息。）

在这个示例中,将单麦克风采集的声音经过降噪处理，得到 16KHz，16bit，单声道的音频数据，并且将降噪后的数据保存到SD card。该示例主要展示单麦克法降噪算法，麦克风阵列语音增强请参考示例[mic_array_speech_enhancement](../mic_array_speech_enhancement).　
"RST"按键，开始录音（重新开始,会覆盖原始录音文件）
"Mode"按键，选择开启或关闭降噪，开启降噪:`NS STATE: 1`, 关闭降噪：`NS STATE: ０`。 　　
"Rec"按键，结束录音。　　　

## 如何使用例程

### Hardware Required
### 硬件需求

- 这个示例能够在 ESP32-LyraT-Mini 开发板或 ESP32-Korvo V1.1 开发板上运行，关于开发板更多的信息，请参考 [ESP32-LyraT-Mini Getting Started Guide](https://docs.espressif.com/projects/esp-adf/en/latest/get-started/get-started-esp32-lyrat-mini.html) 或者 [ESP32-Korvo Getting Started Guide](https://github.com/espressif/esp-skainet/blob/master/docs/en/hw-reference/esp32/user-guide-esp32-korvo-v1.1.md)
- SD card

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

- 上电打印：

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

- 按压 `Mode` 按钮，关闭或重新开启降噪   
- 按压 `Rec` 按钮，结束录音,录音文件"TEST.WAV"保存与SD card根目录．*注意：如果不按`Rec`按钮，直接拔出SD card，录音文件的编码会出现错误．*   
- 按压 `RST` 按钮，重新开始录音  