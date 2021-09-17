cmake_minimum_required(VERSION 3.5)

set(EXTRA_COMPONENT_DIRS
    ../../components
    $ENV{IDF_PATH}/examples/common_components/led_strip
    )

include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(speech_commands_recognition)
