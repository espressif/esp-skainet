# 中文语言合成示例 

本示例主要用于展示乐鑫中文语音合成库的使用方法。示例可通过URAT串口输入中文文本，回车后播放合成的语音数据。
参考`components/esp-tts` 目录下的 [README.md](https://github.com/espressif/esp-sr/blob/release/v1.0/esp-tts/README.md) 来获取更多有关乐鑫中文语言合成库的信息。


### 额外硬件需求

- 一只外接喇叭(4~6 欧姆)


### 编译和烧写
- 参考partition.csv中的第二行添加voice data
```
# Name,  Type, SubType, Offset,  Size
factory, app,  factory, 0x010000, 4M
voice_data, data,  fat,         , 3M
```

- 编译和烧写程序
```
# flash app bin and voice_data 
idf.py flash monitor

# only flash app bin
idf.py app-flash monitor

```

### 修改voice data
//@todo this section should be updated
所有可用voice data放置在 `esp-skainet/components/esp-sr/esp-tts/esp_tts_chinese/`
- 方法1. 修改CmakeLists.txt   
修改CmakeLists.txt中 `voice_data_image` 的路径
```
set(voice_data_image ${PROJECT_DIR}/../../components/esp-sr/esp-tts/esp_tts_chinese/esp_tts_voice_data_xiaoxin_small.dat)
```

- 方法2. 单独烧写 voice data    
使用该例子提供的 `flash_voicedata.sh` 脚本烧写相应的音频数据
```
source flash_voicedata.sh ../../components/esp-sr/esp-tts/esp_tts_chinese/esp_tts_voice_data_xiaoxin_small.dat  /dev/ttyUSB0
```

### 例程输出

上电后，开发板会播放提示音：“欢迎使用乐鑫语音合成”  
并输出以下打印：

```
欢迎使用乐鑫语音合成
I (266) tts_parser: unicode:0x6b22 -> huan1
I (266) tts_parser: unicode:0x8fce -> ying2
I (276) tts_parser: unicode:0x4f7f -> shi3
I (276) tts_parser: unicode:0x7528 -> yong4
I (286) tts_parser: unicode:0x4e50 -> le4
I (286) tts_parser: unicode:0x946b -> xin1
I (296) tts_parser: unicode:0x8bed -> yu3
I (296) tts_parser: unicode:0x97f3 -> yin1
I (306) tts_parser: unicode:0x5408 -> he2
I (306) tts_parser: unicode:0x6210 -> cheng2

请输入短语:
```

### URAT输入
通过串口工具输入文本，回车播放。 **注意**:部分串口工具不支持中文输入。   
如果使用minicom，运行`minicom -s`进入设置页面, 进行以下设置：   

 - Serial port setup -> Serial Device -> /dev/ttyUSB0 (设置串口号)
 - Serial port setup -> Hardware Flow Control -> No
 - Screen and keyboard -> Local echo -> No

minicom显示有一些bug，但可以正确输入中文字符， 比如输入: “大家好”，回车出现以下打印：
```
uart input: ��大��家��好
tts input:��大��家��好
I (5266) tts_parser: unicode:0x5927 -> da4
I (5266) tts_parser: unicode:0x5bb6 -> jia1
I (5276) tts_parser: unicode:0x597d -> hao3

��请��输��入��短��语
```

如果大家有更好的串口工具，欢迎留言。
