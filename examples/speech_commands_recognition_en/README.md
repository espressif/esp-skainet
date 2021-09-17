**# English Speech Commands Recognition**



(See the [README.md](../README.md) file in the upper level 'examples' directory for more information about examples.)



In this example, we have configured 32 command IDs to represent:

0,tell me a joke

1,sing a song

2,play news channel

3,turn on my soundbox

4,turn off my soundbox

5,highest volume

6,lowest volume

7,increase the volume

8,decrese the volume

9,turn on the tv

10,turn off the tv

11,make me a tea

12,make me a coffee

13,turn on the light

14,turn off the light

15,change the color to red

16,change the color to green

17,turn on all the lights

18,turn off all the lights

19,turn on the air conditioner

20,turn off the air conditioner

21,set the temperature to sixteen degrees

22,set the temperature to seventeen degrees

23,set the temperature to eighteen degrees

24,set the temperature to nineteen degrees

25,set the temperature to twenty degrees

26,set the temperature to twenty one degrees

27,set the temperature to twenty two degrees

28,set the temperature to twenty three degrees

29,set the temperature to twenty four degrees

30,set the temperature to twenty five degrees

31,set the temperature to twenty six degrees



**## How to use this example**



**### Hardware Required**



\- This example can run on ESP32-S3-Korvo V4.0 or ESP-BOX. 

\- A Speaker



**### Configure, Build and Flash**



##### set-target 

```
idf.py set-target esp32s3
```

##### configure

Select the default sdkconfig according to the development board module

```
cp sdkconfig_esp32s3r8_8+4.defaults sdkconfig
```

##### build&flash

Build the project and flash it to the board, then run the monitor tool to view the output via serial port:

```
idf.py -b 2000000 flash monitor 
```

(To exit the serial monitor, type ``Ctrl-]``.)

##### check the version of models

To ensure that the example is configured correctly, please check the version of wakenet, multinet and AFE

| model name |                           version                            |
| ---------- | :----------------------------------------------------------: |
| wakenet    | wakeNet8_v2_alexa_5_0.58_0.55, mode:2, p:3, (Sep 14 2021 11:03:51) |
| multinet   |     MN5Q8_v1_english_8_0.9_0.90, (Sep 15 2021 17:01:50)      |
| AFE        | TWO-MIC auido front-end for speech recognition, mode:0, (Sep 14 2021 11:03:54) |



