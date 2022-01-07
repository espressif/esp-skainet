set(COMPONENT_SRCS
    main.c
    tts_urat.c
    )

set(COMPONENT_ADD_INCLUDEDIRS 
    include
    )

set(COMPONENT_REQUIRES
    esp-sr
    hardware_driver
    sr_ringbuf
    player
    )

register_component()



