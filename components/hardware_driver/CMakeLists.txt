if(CONFIG_ESP32_S3_KORVO_1_V4_0_BOARD)
    list(APPEND BSP_BOARD_SRC "./boards/esp32s3-korvo-1")
endif()

if(CONFIG_ESP32_S3_KORVO_2_V3_0_BOARD)
    list(APPEND BSP_BOARD_SRC "./boards/esp32s3-korvo-2")
endif()

if(CONFIG_ESP32_KORVO_V1_1_BOARD)
    list(APPEND BSP_BOARD_SRC "./boards/esp32-korvo")
endif()

if(CONFIG_ESP32_S3_BOX_BOARD)
    list(APPEND BSP_BOARD_SRC "./boards/esp32s3-box")
endif()

if(CONFIG_ESP32_S3_EYE_BOARD)
    list(APPEND BSP_BOARD_SRC "./boards/esp32s3-eye")
endif()

idf_component_register(
    SRC_DIRS
        "./"
        "./led"
        ${BSP_BOARD_SRC}
        "codec/es7210"
        "codec/es8311"
        "codec/es8388"
        "i2c_bus"
    INCLUDE_DIRS
        "include"
        "codec/include"
        "codec/es7210"
        "codec/es8311"
        "codec/es8388"
        "i2c_bus/include"
        "boards/include"
        "./led/include"
    REQUIRES
        fatfs
        spiffs)