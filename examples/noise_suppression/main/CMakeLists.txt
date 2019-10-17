set(COMPONENT_SRCS
    codec_init.c
    main.c
    )

set(COMPONENT_ADD_INCLUDEDIRS 
    include
    )

set(COMPONENT_REQUIRES
    esp-sr
    hardware_driver
    esp_adc_cal
    )

register_component()



