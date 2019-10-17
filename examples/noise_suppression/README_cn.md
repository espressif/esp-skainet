# 降噪例程 [[English]](./README.md)

（参考在上一级的 `examples` 目录下的 [README.md](../README.md) 文件来获取更多信息。）

在这个示例中,将麦克风采集的声音经过降噪处理，得到 16KHz，16bit，单声道的音频数据，并且立即将降噪后的数据输出。用户可以通过通过按下 LyraT_Mini 开发板上的 "Mode" 按键来选择降噪是否打开。
 
## 如何使用例程

### Hardware Required
### 硬件需求

- 这个示例能够在 ESP32-LyraT-Mini 开发板上运行，关于开发板更多的信息，请参考 [ESP32-LyraT-Mini Getting Started Guide](https://docs.espressif.com/projects/esp-adf/en/latest/get-started/get-started-esp32-lyrat-mini.html).

- 耳机

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
NS STATE: 1
```

表示初始状态为降噪打开的状态。当插上耳机时，就可以听到经过降噪处理后的声音。


```
NS STATE: 0
```

表示目前状态为降噪关闭的状态。再次按下 "Mode" 键则又会进入降噪打开的模式。