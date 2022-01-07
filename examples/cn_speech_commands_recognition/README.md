# 中文语音命令识别 

（参考在上一级的 `examples` 目录下的 [README.md](../README.md) 文件来获取更多信息。）

在这个示例中，我们配置了 17 个命令词 ID：
```
0. 打开空调
1. 关闭空调
2. 增大风速
3. 减小风速
4. 升高一度
5. 降低一度
6. 制热模式
7. 制冷模式
8. 节能模式
9. 关闭节能模式
10. 除湿模式
11. 关闭除湿模式
12. 睡眠模式
13. 关闭睡眠模式
14. 定时一小时
15. 定时两小时
16. 最大风速
17. 最高风速
```

我们已经通过 `menuconfig` 配置了 45 类常见的垃圾名称。

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

## 例程输出

以下是例程的上电打印输出:

```
accelate model: 1
MC Quantized wakeNet8: wakeNet8_v2_alexa_5_0.58_0.55, mode:2, p:3, (Sep 14 2021 11:03:51)
Initial TWO-MIC auido front-end for speech recognition, mode:0, (Sep 14 2021 11:03:54)
layer0 6 26
layer1 8 64
layer2 6 96
layer3 6 128
layer4 4 264
SINGLE_RECOGNITION: V3.0 CN; core: 0; (Sep 15 2021 17:01:49)
I (2348) MN: ---------------------SPEECH COMMANDS---------------------
I (2358) MN: Command ID0, phrase 0: da kai kong tiao
I (2368) MN: Command ID1, phrase 1: guan bi kong tiao
I (2368) MN: Command ID2, phrase 2: zeng da feng su
I (2378) MN: Command ID3, phrase 3: jian xiao feng su
I (2378) MN: Command ID4, phrase 4: sheng gao yi du
I (2388) MN: Command ID5, phrase 5: jiang di yi du
I (2388) MN: Command ID6, phrase 6: zhi re mo shi
I (2398) MN: Command ID7, phrase 7: zhi leng mo shi
I (2408) MN: Command ID8, phrase 8: jie neng mo shi
I (2408) MN: Command ID9, phrase 9: guan bi jie neng mo shi
I (2418) MN: Command ID10, phrase 10: chu shi mo shi
I (2418) MN: Command ID11, phrase 11: guan bi chu shi mo shi
I (2428) MN: Command ID12, phrase 12: shui mian mo shi
I (2438) MN: Command ID13, phrase 13: guan bi shui mian mo shi
I (2438) MN: Command ID14, phrase 14: ding shi yi xiao shi
I (2448) MN: Command ID15, phrase 15: ding shi liang xiao shi
I (2448) MN: Command ID16, phrase 16: zui da feng su
I (2458) MN: Command ID16, phrase 17: zui gao feng su
I (2468) MN: ---------------------------------------------------------

------------detect start------------

