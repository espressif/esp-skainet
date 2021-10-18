set(COMPONENT_SRCS
    main.c
    )

set(COMPONENT_REQUIRES
    esp-sr
    hardware_driver
    spiffs
    )

set(COMPONENT_ADD_INCLUDEDIRS 
    include
    )

register_component()

