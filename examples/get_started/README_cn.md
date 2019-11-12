# ESP-Skainet 使用指南 [[English]](./README.md)

目前，基于乐鑫 ESP32 的语音命令词识别模型 [MultiNet](https://github.com/espressif/esp-sr/tree/master/speech_command_recognition/README_cn.md) 支持 100 个以内的中文或英文自定义命令词。

这个示例展示了使用 ESP32-LyraT-Mini 或 ESP32-LyraT V4.3 进行语音命令词识别的基本流程。请参考以下流程：

![speech-commands-recognition-system](../../img/speechs_commands_workflow.png)  

获取更多关于 ESP32-LyraT-Mini 或者 ESP32-LyraT V4.3 的信息，请参考 [ESP32-LyraT-Mini Getting Started Guide](https://docs.espressif.com/projects/esp-adf/en/latest/get-started/get-started-esp32-lyrat-mini.html) 或者 [ESP32-LyraT V4.3 Getting Started Guide](https://docs.espressif.com/projects/esp-adf/en/latest/get-started/get-started-esp32-lyrat.html)

# 1. 快速开始

### 1.1 硬件配置

  如果使用 ESP32-Lyrat Mini V1.1 开发板，进入 `Audio Media HAL`，按照指示配置以下参数：  
  `Audio hardware board`: 选择 `ESP32-Lyrat Mini V1.1`;  

  ![speech-commands-recognition-system](../../img/specch_commands_config1.png)  

  如果使用 ESP32-LyraT V4.3 开发板，进入 `Audio Media HAL`，按照指示配置以下参数：  
  `Audio hardware board`: 选择 `ESP32-LyraT V4.3`;  
  

### 1.2 软件配置

  进入 `ESP Speech Recognition`，按照指示配置以下参数： 
  - `Wake word engine`: 选择 `WakeNet 5 (quantized)`;
  - `Wake word name`: 选择 `hilexin (WakeNet5)`;
  - `speech commands recognition model to us`: 选择 `MultiNet 1 (quantized)`;
  - `langugae`: 选择 `chinese (MultiNet1)`，如果使用英文命令词识别，请选择 `english (MultiNet1)`;
  - `The number of speech commands`-> The number of speech commands ID;
  - `Add speech commands`-> Add the speech commands.

  ![speech-commands-recognition-system](../../img/specch_commands_config2.png)  

 最后保存退出即可。

### 1.3 添加自定义命令词

目前，MultiNet 模型中已经预定义了一些命令词。用户可以通过 `menuconfig -> Component config -> ESP Speech Recognition -> Add speech commands` and `The number of speech commands`来定义自己的语音命令词和语音命令的数目。

#### 1.3.1 中文命令词识别

在填充命令词时应该使用拼音，并且每个字的拼音拼写间要间隔一个空格。比如“打开空调”，应该填入 "da kai kong tiao".

#### 1.3.2 英文命令词识别

在填充命令词时应该使用特定音标，请使用 skainet 根目录 `tools` 目录下的 `general_label_EN/general_label_en.py` 脚本生成命令词对应的音标，具体使用方法请参考 [音标生成方法](../../tools/general_label_EN/README.md) .

**注意：**
- 一个语音命令 ID 可以对应多条语音指令短语；
- 最多支持 100 个语音命令 ID 或者命令短语；
- 同一个语音命令 ID 对应的多条语音指令短语之间要使用“,”隔开

### 1.4 搭建针对语音命令的动作函数

用户可以通过 `void speech_commands_action(int command_id)` 函数定义针对每个语音命令的动作，比如：

```
void speech_commands_action(int command_id)
{
    printf("Commands ID: %d.\n", command_id);
    switch (command_id) {
    case 0:
        // action0();
        break;
    case 1:
        // action1();
        break;
    case 2:
        // action2();
        break;
    case 3:
        // action3();
        break;
    // ...
    default:
        break;
    }
}
```

### 1.5 编译和运行

当我们选择使用中文命令词识别，运行 `make flash monitor` 来编译烧写该示例，并且检查以下输出打印：

```
Quantized wakeNet5: wakeNet5_v1_hilexin_5_0.95_0.90, mode:0
Quantized MN1
I (153) MN: ---------------------SPEECH COMMANDS---------------------
I (163) MN: Command ID0, phrase 0: da kai kong tiao
I (163) MN: Command ID1, phrase 1: guan bi kong tiao
I (173) MN: Command ID2, phrase 2: da kai dian deng
I (173) MN: Command ID3, phrase 3: guan bi dian deng
I (183) MN: ---------------------------------------------------------

chunk_num = 200
-----------awaits to be waken up-----------
```

### 1.6 唤醒板子

可以通过板子的输出打印找到支持的唤醒词。在这个示例中，唤醒词是 “Hi Lexin" [Ləsɪ:n]. 

然后，说出 “Hi Lexin" ([Ləsɪ:n]) 来唤醒板子，唤醒后打印如下信息：

```
hilexin DETECTED.
-----------------LISTENING-----------------
```

### 1.7 语音命令词识别

然后，板子会进入侦听状态，等待语音命令词。

目前，MultiNet 已经预定义了 20 个词，可以参考 [MultiNet](https://github.com/espressif/esp-sr/tree/master/speech_command_recognition/README.md).

* 如果命令词存在于命令词列表中，回打印如下 log：

	```
	-----------------LISTENING-----------------
    
    phrase ID: 0, prob: 0.866630
    Commands ID: 0
    
    -----------awaits to be waken up-----------

	```

* 如果命令词不存在于命令词列表中，回打印如下 log：

	```
	-----------------LISTENING-----------------
    
	cannot recognize any speech commands
    
	-----------awaits to be waken up-----------

	```

当板子结束当前的识别过程并且进入等待唤醒状态时，会打印： `-----------awaits to be waken up-----------` 

**注意：**
板子会在侦听状态最长持续 6 s，之后，会结束当前的识别过程并且再次进入等待唤醒状态。因此，用户必须在板子被唤醒后 6 s 内发出语音命令词。

# 2. 工作流程

### 2.1 硬件初始化

用户不需要任何特殊用途的开发板即可运行** WakeNet **和** MultiNet **示例。 目前，乐鑫已经发布了数种音频开发板，其中 ESP32-LyraT-Mini 和 ESP32-LyraT V4.3 是我们在本示例中使用的开发板。 

关于 ESP32-LyraT-Mini 开发板和 ESP32-LyraT V4.3 开发板的初始化，请参考 `components/hardware_driver` 的代码。

如果用户选择其他的开发板，请参考 [esp-adf](https://github.com/espressif/esp-adf)， ESP-ADF 可以提供更多关于硬件驱动和应用搭建的细节。

### 2.2 唤醒

板子在等待唤醒状态时，需要向 **WakeNet** 逐帧输入来自板载麦克风的音频数据流，需要满足（30 ms, 16 KHz, 16 bit, mono）的格式。

目前，用户不能自己自定义唤醒词，如果有自定义唤醒词的需求，请参考[乐鑫语音唤醒词定制流程](wake_word_engine/乐鑫语音唤醒词定制流程.md)。

### 2.3 语音命令词识别

在识别阶段，需要向 **MultiNet** 逐帧输入来自板载麦克风的音频数据流，需要满足（30 ms, 16 KHz, 16 bit, mono）的格式。然后 MultiNet 会将接收到的命令词和预存的命令词列表中的命令词进行对比，并且返回识别结果。

关于如何自定义语音命令词参考 1.5 小节。