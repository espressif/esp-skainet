# Wake Word Detection



(See the [README.md](../README.md) file in the upper level 'examples' directory for more information about examples.)

| Supported Targets | ESP32    | ESP32-S3 | ESP32-P4 | 
| ----------------- | -------- | -------- | -------- |

This example is used to test performance of wakenet.(the word word engine of Espressif).
This example can load multiple models, but can only run two models at the same time


### Configure

Select board and wake words
```
idf.py set-target esp32s3
idf.py menuconfig

# Select audio board
Audio Media HAL -> Audio hardware board -> ESP32-S3-Korvo-1

# Load one wakenet model
ESP Speech Recognition -> Select wake words -> Hi,Lexin (wn9_hilexin)

# Load multiple wakenet models
ESP Speech Recognition -> Select wake words -> Hi,Lexin (wn9_hilexin) -> Load Multiple Wake Words
ESP Speech Recognition -> Load Multiple Wake Words -> Hi,Lexin (wn9_hilexin)
                                                   -> Hi,ESP (wn9_hiesp)
```

### build&flash

Build the project and flash it to the board, then run the monitor tool to view the output via serial port:

```
idf.py flash monitor 
```

(To exit the serial monitor, type ``Ctrl-]``.)


### modify detection threshold

The default detection threshold is defined in `_MODEL_INFO_` file, and it will be printed when model is created. 

For example, [`wakenet9_v1h24_嗨，乐鑫_3_0.608_0.615`](https://github.com/espressif/esp-sr/blob/master/model/wakenet_model/wn9_hilexin/_MODEL_INFO_) has a detection threshold range from 0.608 to 0.615. 
You can modify threshold in the following ways:

- modify the threshold in `_MODEL_INFO_` file

- modify the threshold in code

```
afe_handle->set_wakenet_threshold(afe_handle, model_index, threshold); // currently AFE support to load two model. model_index is 1 or 2
afe_handle->reset_wakenet_threshold(afe_handle, model_index);          // reset threshold to default
```

**Note**: This API is supported from esp-sr v2.1.3