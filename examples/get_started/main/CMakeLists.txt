set(COMPONENT_SRCS
    codec_init.c
    main.c
    recsrc.c
    speech_commands_action.c
    )

set(COMPONENT_ADD_INCLUDEDIRS 
    include
    )

set(COMPONENT_REQUIRES
    esp-sr
    hardware_driver
    )

register_component()



