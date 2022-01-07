# Wake Word Detection



(See the [README.md](../README.md) file in the upper level 'examples' directory for more information about examples.)

This example is used to test performance of wakenet.(the word word engine of Espressif).


## How to use this example

### Configure, Build and Flash


##### set-target 

```
idf.py set-target esp32s3
```

##### configure

Select the default sdkconfig according to the development board module

```
cp sdkconfig.defaults.esp32s3 sdkconfig
```

Select the different wake word
```
idf.py menuconfig
ESP Speech Recognition -> Wake word engine    // select the version of wakenet
ESP Speech Recognition -> Wake word name      // select the wake word
```

##### build&flash

Build the project and flash it to the board, then run the monitor tool to view the output via serial port:

```
idf.py -b 2000000 flash monitor 
```

(To exit the serial monitor, type ``Ctrl-]``.)


