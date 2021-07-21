if(IDF_TARGET STREQUAL "esp32s3")
    set(COMPONENT_SRCS
        main.c
        speech_commands.c
        )

    set(COMPONENT_REQUIRES
        hardware_driver
        )
endif()

set(COMPONENT_ADD_INCLUDEDIRS 
    include
    )

register_component()
