# 中文语音命令识别 

（参考在上一级的 `examples` 目录下的 [README.md](../README.md) 文件来获取更多信息。）

## 如何使用例程

### Hardware Required
### 额外硬件需求

- 一只外接喇叭(4~6 欧姆)

### 配置工程

* 根据使用的开发板模组选择对应的 `sdkconfig`

* 进入 `idf.py menuconfig`

* 通过 `Serial Flasher Options`设置串口信息

### 编译和烧写

编译并烧写，然后运行终端监控查看打印：

```
idf.py flash monitor
```

(退出窗口，请键入 ``Ctrl-]``.)

参考 [Getting Started Guide](https://docs.espressif.com/projects/esp-idf/en/stable/get-started-cmake/index.html) 来获取更多使用 ESP-IDF 编译项目的细节.

### 修改命令词

我们推荐使用MultiNet6或更新的模型。  
以下是一个简单的例子展示如何在代码中修改命令词，你也可以通过修改默认的命令词列表文件修改命令，具体请参考：[命令词识别文档](https://docs.espressif.com/projects/esp-sr/zh_CN/latest/esp32s3/speech_command_recognition/README.html).

```
// MultiNet6
    // Note: Please create multinet handle before adding speech commands

    esp_mn_commands_clear();                       // 清除当前的命令词列表
    esp_mn_commands_add(1, "turn on the light");   // 增加一个命令
    esp_mn_commands_add(2, "turn off the light");  // 增加一个命令
    esp_mn_commands_update();                      // 更新命令词列表
    multinet->print_active_speech_commands(model_data);     // 打印当前正在使用的所有命令词条
```
