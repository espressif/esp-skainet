if(IDF_TARGET STREQUAL "esp32")
set(COMPONENT_SRCDIRS 
    ./MediaHal
    ./MediaHal/Codec
    ./SystemSal
    ./i2c_bus
    ./SDCardConfig
    ./button
    ./led
    )

set(COMPONENT_ADD_INCLUDEDIRS 
    ./MediaHal/Board
    ./MediaHal/Codec
    ./MediaHal
    ./SDCardConfig
    ./SystemSal
    ./i2c_bus
    ./userconfig
    ./button/include
    ./led/include
    )

set(COMPONENT_REQUIRES
    fatfs
    esp_adc_cal
    nvs_flash
    )
endif()

register_component()


