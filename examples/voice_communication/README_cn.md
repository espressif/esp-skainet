# 语音例程 [[English]](./README.md)

（参考在上一级的 `examples` 目录下的 [README.md](../README.md) 文件来获取更多信息。）

在这个示例中，我们演示了语音通话的调用方式

## 如何使用例程

### 硬件需求

- 这个示例能够在 ESP32-Korvo 开发板/ESP32-S3-Korvo 开发板上运行，关于开发板更多的信息，请参考 [ESP32-Korvo Getting Started Guide](https://github.com/espressif/esp-skainet/blob/master/docs/en/hw-reference/esp32/user-guide-esp32-korvo-v1.1.md) 或者 [ESP32-S3-Korvo-1 Getting Started Guide](https://github.com/espressif/esp-skainet/blob/master/docs/en/hw-reference/esp32s3/user-guide-korvo-1.md).


### 配置工程

* 根据使用的开发板模组选择对应的 `sdkconfig`
  
  - 比如， 使用 ESP32， 运行 `cp sdkconfig.defaults.esp32 sdkconfig`

  - 比如， 使用 ESP32-S3， 运行 `cp sdkconfig.defaults.esp32s3 sdkconfig`

  - 比如， 使用 ESP32-P4， 运行 `cp sdkconfig.defaults.esp32p4 sdkconfig`
  
* 进入 `idf.py menuconfig`

* 通过 `Serial Flasher Options`设置串口信息

### 编译和烧写

编译并烧写，然后运行终端监控查看打印：

```
idf.py -b 2000000 flash monitor -p PORT
```

(退出窗口，请键入 ``Ctrl-]``.)

参考 [Getting Started Guide](https://docs.espressif.com/projects/esp-idf/en/stable/get-started-cmake/index.html) 来获取更多使用 ESP-IDF 编译项目的细节.

该例程是语音通话的算法处理，需要看算法处理结果的话。可以打开例程中的宏`DEBUG_SAVE_PCM`，插入SD卡，保存原始音频和处理之后的音频，用音频软件打开，即可查看算法效果。