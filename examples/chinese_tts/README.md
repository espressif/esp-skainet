# 中文语言合成示例 

本示例主要用于展示乐鑫中文语音合成库的使用方法。示例可通过URAT串口输入中文文本，回车后播放合成的语音数据。
参考`components/esp-tts` 目录下的 [README.md](../../components/esp-tts/README.md) 来获取更多有关乐鑫中文语言合成库的信息。


### 硬件需求

- ESP32-LyraT-Mini 、ESP32-Korvo V1.1开发板或者 ESP32-LyraT V4.3 开发板上。关于开发板更多的信息，请参考 [ESP32-LyraT-Mini Getting Started Guide](https://docs.espressif.com/projects/esp-adf/en/latest/get-started/get-started-esp32-lyrat-mini.html)， [ESP32-Korvo Getting Started Guide](https://github.com/espressif/esp-skainet/blob/master/docs/en/hw-reference/esp32/user-guide-esp32-korvo-v1.1.md)或者 [ESP32-LyraT V4.3 Getting Started Guide](https://docs.espressif.com/projects/esp-adf/en/latest/get-started/get-started-esp32-lyrat.html).

- 一只外接喇叭(4~6 欧姆)

### 配置工程

* 进入 `idf.py menuconfig`

* 通过 `Serial Flasher Options`设置串口信息


### 编译和烧写

#### 方法一：将voice data编译进入app bin,该方法为默认方法．

##### 1)如果使用ESP32-S2，请确认IDF版本为v4.2或以上;如果使用ESP32-S3，请确认IDF版本为v4.2或以上; 执行以下命令设置编译目标芯片。
```
idf.py set-target esp32/esp32s2/esp32s3
```
##### 2)进入配置界面，选择合适的开发板
```
idf.py menuconfig
```
##### 3)编译烧录程序
```
idf.py flash monitor -p /dev/ttyUSB0

```
参考 [Getting Started Guide](https://docs.espressif.com/projects/esp-idf/en/stable/get-started-cmake/index.html) 来获取更多使用 ESP-IDF 编译项目的细节.

#### 方法二： 将voice data烧写到独立分区，可以有效降低app bin大小，方便用户OTA： 

##### 1)参考partition.csv中第二行添加voice data部分的分区列表, 分区大小至少为3MB:  

```
# Name,  Type, SubType, Offset,  Size
factory, app,  factory, 0x010000, 4M
voice_data, data,  fat,         , 3M
```

##### 2)使用该目录下的flash_voicedata.sh脚本，烧写voice data到指定分区:   

```
source flash_voicedata.sh ../../components/esp-sr/esp-tts/esp_tts_chinese/esp_tts_voice_data_xiaole.dat  /dev/ttyUSB0
```
##### 3)修改[代码](./main/main.c)中voice data的初始化方法,选择method2
```

    // method1: use pre-define xiaole voice lib.
    // This method is not recommended because the method may make app bin exceed the limit of esp32  
    // esp_tts_voice_t *voice=&esp_tts_voice_xiaole;

    // method2: initial voice set from separate voice data partition
    const esp_partition_t* part=esp_partition_find_first(ESP_PARTITION_TYPE_DATA, ESP_PARTITION_SUBTYPE_DATA_FAT, "voice_data");
    if (part==0) { 
        printf("Couldn't find voice data partition!\n"); 
        return 0;
    }
    spi_flash_mmap_handle_t mmap;
    uint16_t* voicedata;
    esp_err_t err=esp_partition_mmap(part, 0, 3*1024*1024, SPI_FLASH_MMAP_DATA, (const void**)&voicedata, &mmap);
    if (err != ESP_OK) {
        printf("Couldn't map voice data partition!\n"); 
        return 0;
    }
    esp_tts_voice_t *voice=esp_tts_voice_set_init(&esp_tts_voice_template, voicedata); 

```

##### 4)按照方法一烧写app bin.




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
