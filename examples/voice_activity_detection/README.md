# Voice Activaty Detection


This example is used to test VAD (Voice Activaty Detection). Now two types of VAD are supported: WebRTC VAD and Espressif's vadnet1. You can select the desired version through menuconfig. Compared to WebRTC noise, vadnet1 can filter out more noise, but it also consumes more CPU resources.


### Configure 

Select board and wake words
```
idf.py set-target esp32s3
idf.py menuconfig

# Select audio board
Audio Media HAL -> Audio hardware board -> ESP32-S3-Korvo-1

# Load vadnet1 model
ESP Speech Recognition -> Select voice activity detection -> voice activity detection (vadnet1 medium)
```

### Setting

You can set the following parameters in the config file:
```
    vad_init;           // Whether to init vad
    vad_mode;           // The value can be: VAD_MODE_0, VAD_MODE_1, VAD_MODE_2, VAD_MODE_3, VAD_MODE_4
                        // The larger the mode, the higher the speech trigger probability.

    vad_model_name;     // The model name of vad, If it is null, WebRTC VAD will be used.
    vad_min_speech_ms;  // The minimum duration of speech in ms. It should be bigger than 32 ms, default: 128 ms
    vad_min_noise_ms;   // The minimum duration of noise or silence in ms. It should be bigger than 64 ms, default: 1000 ms
    vad_delay_ms;       // The delay of the first speech frame in ms, default: 128 ms
                            // If you find vad cache can not cover all speech, please increase this value.
```

There are two issues in the VAD settings that can cause a delay in the first frame trigger of speech.
1. The inherent delay of the VAD algorithm itself. VAD cannot accurately trigger speech on the first frame and may delay by 1 to 3 frames.
2. To avoid false triggers, the VAD is triggered when the continuous trigger duration reaches the `vad_min_speech_ms` parameter in AFE configuation.
Due to the above two reasons, directly using the first frame trigger of VAD may cause the first word to be truncated. 
To avoid this situation, AFE V2.0 has added a VAD cache. You can determine whether a VAD cache needs to be saved by checking the vad_cache_size

### build&flash

Build the project and flash it to the board, then run the monitor tool to view the output via serial port:

```
idf.py flash monitor 
```

(To exit the serial monitor, type ``Ctrl-]``.)


