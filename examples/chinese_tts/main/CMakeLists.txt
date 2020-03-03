set(COMPONENT_SRCS
    main.c
    tts_urat.c
    )

set(COMPONENT_ADD_INCLUDEDIRS 
    include
    )

set(COMPONENT_REQUIRES
    esp-tts
    hardware_driver
    )

register_component()



