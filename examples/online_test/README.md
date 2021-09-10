# Get Started Guide for ESP-Skainet [[中文]](./README_cn.md)

This example demonstrates the basic process of with ESP32-Korvo V1.1,ESP32-S3-Korvo V4.0. 



# 1. Quick Start

### 1.1 Hardware Configuration

  If use ESP32-Lyrat Mini V1.1, Navigate to `Audio Media HAL`, and configure the following parameters as instructed.  
  `Audio hardware board`: select `ESP32-Lyrat Mini V1.1`;  

  ![speech-commands-recognition-system](../../img/audio_boards.png)  

  If use ESP32-Korvo V1.1, Navigate to `Audio Media HAL`, and configure the following parameters as instructed.  
  `Audio hardware board`: select `ESP32-Korvo V1.1`;    

### 1.2 Software Configuration

   Navigate to `Component config` -> `ESP Speech Recognition`, and configure the following parameters as instructed.
  - `Wake word engine`: select `WakeNet 5 (quantized)`;
  - `Wake word name`: select `hilexin (WakeNet5)`;
  - `speech commands recognition model to us`: select `MultiNet 1 (quantized)`;
  - `langugae`: select `chinese (MultiNet3)`, select `english (MultiNet5)` if use english speech command recognition;
  - `The number of speech commands`-> The number of speech commands ID;
  - `Add speech commands`-> Add the speech commands in pinyin or Phonetic symbol.

  ![speech-commands-recognition-system](../../img/specch_commands_config2.png)  

Then save the configuration and exit.

### 1.3 Compiling and Running

When you choose to use Chinese command word recognition, run `ida.py flash monitor` to compile, flash and run this example, and check the output log:

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

### 1.4 Waking up the Board

Find the pre-defined wake word of the board in the printed log. In this example, the wake word is “Hi Lexin" [Ləsɪ:n]. 

Then, say “Hi Lexin" ([Ləsɪ:n]) to wake up the board, which then wakes up and prints the following log:

```
hilexin DETECTED.
-----------------LISTENING-----------------
```

### 1.5 Recognizing Speech Commands

Then, the board enters the Listening status, waiting for new speech commands.

Currently, the MultiNet model already defined 20 speech commands, which can be seen in [MultiNet](https://github.com/espressif/esp-sr/tree/master/speech_command_recognition/README.md). 

Now, you can give one speech command, for example, “打开空调 (turn on the air conditioner)”,

* If this command exists in the supported speech command list, the board prints out the command id of this command in its log: 

	```
	-----------------LISTENING-----------------
    
    phrase ID: 0, prob: 0.866630
    Commands ID: 0
    
    -----------awaits to be waken up-----------

	```
	
* If this command does not exist in the supported speech command list, the board prints an error message of "cannot recognize any speech commands" in its log:   

  ```
  -----------------LISTENING-----------------
  
  cannot recognize any speech commands
  
  -----------awaits to be waken up-----------
  ```

  Also, the board prints `-----------awaits to be waken up-----------` when it ends the current recognition cycle and re-enters the Waiting-for-Wakeup status.

**Notices:** 

The board can only stay in the Listening status for up to six seconds. After that, it ends the current recognition cycle and re-enters the Waiting-for-wakeup status. Therefore, you must give speech commands in six seconds after the board wakes up.
