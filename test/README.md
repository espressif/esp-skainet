# Overview

This part is used to test the performance of esp-skainet, include wakenet, multinet and AFE.

## 1. build all test app
'''
pip install -r tools/ci/requirement.txt

python tools/ci/build_apps.py ./test -t esp32s3 -vv
'''

This script will compile all apps in the `esp-skainet/test`, and generate different bins according to different sdkconfig.ci.***.  
For example, there are `sdkconfig.ci.hilexin` and `sdkconfig.ci.hiesp` in `esp-skainet/test/wakenet`. So after running the above script, build_esp32s3_hilexin and build_esp32s3_hiesp will be generated. 

build rules are determined in `esp-skainet/test/.build-relus.yml`. You can modify this yaml file to set which apps should be build.

## 2. run and report

```
# test wakenet performance

pytest ./test/wakenet --target esp32s3
```
The test report will be generated in `./pytest_log/`.