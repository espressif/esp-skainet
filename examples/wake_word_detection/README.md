# Wake Word Detection



(See the [README.md](../README.md) file in the upper level 'examples' directory for more information about examples.)

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


