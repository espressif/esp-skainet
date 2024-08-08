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

if(CONFIG_ESP32_S3_BOX_3_BOARD)
    list(APPEND BSP_BOARD_SRC "./boards/esp32s3-box-3")
endif()

if(CONFIG_ESP32_S3_EYE_BOARD)
    list(APPEND BSP_BOARD_SRC "./boards/esp32s3-eye")
endif()

if(CONFIG_ESP32_P4_FUNCTION_EV_BOARD)
    list(APPEND BSP_BOARD_SRC "./boards/esp32p4-function-ev")
endif()


idf_component_register(
    SRC_DIRS
        ./
        ${BSP_BOARD_SRC}
    INCLUDE_DIRS
        "include"
        "boards/include"
    REQUIRES
        driver
        fatfs
        spiffs
        )

component_compile_options(-w)
target_compile_options(${COMPONENT_LIB} PRIVATE "-Wno-format")
