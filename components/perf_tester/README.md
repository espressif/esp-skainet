Perf tester is used to test esp-skainet performance. Currently, it support wakenet and multinet test.
Perf tester use [console](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/system/console.html) component to develop an interactive console over serial port. The commands list are as follows:
```
help 
  Print the list of registered commands

config  <fast/norm>  <all/none/pink/pub>  <all/none/0/5/10>
  Perf Tester Config
   <fast/norm>  Tester mode, fast: fast check all test cases, norm: run complete test cases.
   <all/none/pink/pub>  noise type, All: test all noise data, none: do not add any noise
   <all/none/0/5/10>  snr, all: test all snr data, none: do not add any noise, 0/5/10: snr number

start 
  Start to test
```
You can configure and run tests by the following steps:

```
#select pink noise and SNR=5dB, fast test mode
perf_test>config fast pink 5
perf_test>start


#select all noise and SNR=5dB, fast test mode
perf_test>config fast all 5
perf_test>start

#select pub noise and all SNR, fast test mode
perf_test>config fast pub all
perf_test>start

```

You can refer to [this example](https://github.com/espressif/esp-idf/tree/master/examples/system/console) to register new commands.