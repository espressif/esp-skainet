set(COMPONENT_SRCS
    main.c
    speech_commands_action.c
    )

set(COMPONENT_ADD_INCLUDEDIRS 
    include
    )

set(COMPONENT_REQUIRES
    esp-sr
    led_strip
    hardware_driver
    spiffs
    )

register_component()



