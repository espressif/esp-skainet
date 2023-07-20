# Overview

This part is used to test the performance of esp-skainet, include wakenet, multinet and AFE modules.

## How to use it
### 1. build all test app
```
pip install -r tools/ci/requirement.txt

python tools/ci/build_apps.py ./test -t esp32s3 -vv
```

This script will build all apps in the `esp-skainet/test`, and generate different bins according to different sdkconfig.ci.***.  
For example, there are `sdkconfig.ci.hilexin` and `sdkconfig.ci.hiesp` in `esp-skainet/test/wakenet`. So after running the above script, `esp-skainet/test/wakenet/IDF_VERSION/build_esp32s3_hilexin` and `esp-skainet/test/wakenet/IDF_VERSION/build_esp32s3_hiesp` will be generated. IDF_VERSION is your idf verison.

build rules are determined in `esp-skainet/test/.build-relus.yml`. You can modify this yaml file to set which targets should be build.

### 2. run and report

```
# test wakenet performance

pytest ./test/wakenet --target esp32s3 --config hilexin --noise all|pink|pub|none --snr all|0|5
```
The test report will be generated in `./pytest_log/`.

## How to add a test case

### 1. record test set
Please refer to esp-skainet/tools/record_test_set to record your test case.
### 2. add a test app
Please refer to esp-skainet/test/wakenet to add your test app.

### 3. add a pytest script
Please refer to esp-skainet/test/wakenet/pytest_wakenet.py to add your pytest script.   
The more details about pytest you can find [here](https://espressif-docs.readthedocs-hosted.com/projects/pytest-embedded/en/latest/index.html).
### 4. add a ci pipeline 
Please refer to the following template to add your test case in .gitlab-ci.yml:
```
test_wakenet:
  extends:
    - .pytest_template
  needs:
    - job: "build_all_tests"
      artifacts: true
      optional: true
  tags: 
    - 'korvo-2'
  image: $DOCKER_TARGET_TEST_v5_0_ENV_IMAGE
  variables:
    TEST_TARGET: 'esp32s3'
    TEST_FOLDER: './test/wakenet'
    TEST_ENV: 'korvo-2'
    IDF_VERSION: "5.0"
```