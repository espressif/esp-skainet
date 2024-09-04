# Voice Example [[中文]](./README_cn.md)



(See the [README.md](../README.md) file in the upper level 'examples' directory for more information about examples.)

This example is used to test performance of voice communication.


## How to use this example

### Hardware Required

- This example can be run on the esp32 korvo development board / esp32-s3-korvo development board. For more information about the development board, please refer to  [ESP32-Korvo Getting Started Guide](https://github.com/espressif/esp-skainet/blob/master/docs/en/hw-reference/esp32/user-guide-esp32-korvo-v1.1.md) or [ESP32-S3-Korvo-1 Getting Started Guide](https://github.com/espressif/esp-skainet/blob/master/docs/en/hw-reference/esp32s3/user-guide-korvo-1.md).


##### configure

* Select the default sdkconfig according to the development board module

- For example, using ESP32, run `cp sdkconfig.defaults.esp32 sdkconfig`

- For example, using ESP32-S3, run `cp sdkconfig.defaults.esp32s3 sdkconfig`

- For example, using ESP32-P4, run `cp sdkconfig.defaults.esp32p4 sdkconfig`

* Run  `idf.py menuconfig`

* Set serial port information through `Serial Flasher Options`

##### build&flash

Build the project and flash it to the board, then run the monitor tool to view the output via serial port:

```
idf.py -b 2000000 flash monitor -p PORT
```

(To exit the serial monitor, type ``Ctrl-]``.)

If you want to get more detail of build ESP-IDF, please refer to [Getting Started Guide](https://docs.espressif.com/projects/esp-idf/en/stable/get-started-cmake/index.html)

This example is the algorithm processing of voice communication. If you need to see the result of algorithm processing. You can open the macro `DEBUG_SAVE_PCM` in the example, and insert the SD card to save the original audio and processed audio. Then you open it with audio software to view the algorithm effect.
