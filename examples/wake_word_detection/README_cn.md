# 唤醒词使用 [[English]](./README.md)

（参考在上一级的 `examples` 目录下的 [README.md](../README.md) 文件来获取更多信息。）

在这个示例中,麦克风采集的声音首先语音信号增强模块，然后将处理后的语言传递给Wake Word Engine模块进行唤醒词检测。如果检测到唤醒词，会返回唤醒词的编号并打印唤醒词。  
该示例主要用于唤醒词性能测试。
 
## 如何使用例程

### Hardware Required
### 硬件需求

- 这个示例能够在 ESP32-LyraT-Mini 开发板或 ESP32-LyraT V4.3 开发板上运行，关于开发板更多的信息，请参考 [ESP32-LyraT-Mini Getting Started Guide](https://docs.espressif.com/projects/esp-adf/en/latest/get-started/get-started-esp32-lyrat-mini.html) 或者 [ESP32-LyraT V4.3 Getting Started Guide](https://docs.espressif.com/projects/esp-adf/en/latest/get-started/get-started-esp32-lyrat.html)


### 配置工程

* 进入 `make menuconfig`

* 通过 `Serial Flasher Options`设置串口信息

### 编译和烧写

编译并烧写，然后运行终端监控查看打印：

```
make -j4 flash monitor
```

(退出窗口，请键入 ``Ctrl-]``.)

参考 [Getting Started Guide](https://docs.espressif.com/projects/esp-idf/en/stable/get-started-cmake/index.html) 来获取更多使用 ESP-IDF 编译项目的细节.

## 例程输出

上电打印：

```
WakeNet5X3_v2_hilexin_5_0.97_0.90
```

唤醒后打印(time: wake word DETECTED.)：

1.00s: hilexin DERECTED.

