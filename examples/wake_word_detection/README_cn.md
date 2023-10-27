# 唤醒词检测 [[English]](./README.md)

（参考在上一级的 `examples` 目录下的 [README.md](../README.md) 文件来获取更多信息。）


该示例用于展示如何使用 WakeNet 进行唤醒词检测。示例支持同时加载多个 wakenet 模型，但最多选择两个模型进行识别。

### 编译和烧写

开发板和唤醒词配置

```
idf.py set-target esp32s3
idf.py menuconfig

# 选择开发板
Audio Media HAL -> Audio hardware board -> ESP32-S3-Korvo-1

# 加载一个唤醒词
ESP Speech Recognition -> Select wake words -> Hi,Lexin (wn9_hilexin)

# 加载多个唤醒词
ESP Speech Recognition -> Select wake words -> Hi,Lexin (wn9_hilexin) -> Load Multiple Wake Words
ESP Speech Recognition -> Load Multiple Wake Words -> Hi,Lexin (wn9_hilexin)
                                                   -> Hi,ESP (wn9_hiesp)
```        


编译并烧写，然后运行终端监控查看打印：

```
idf.py flash monitor
```

(退出窗口，请键入 ``Ctrl-]``.)

参考 [Getting Started Guide](https://docs.espressif.com/projects/esp-idf/en/stable/get-started-cmake/index.html) 来获取更多使用 ESP-IDF 编译项目的细节.
