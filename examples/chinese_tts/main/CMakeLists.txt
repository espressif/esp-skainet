set(srcs
    main.c
    tts_urat.c
    )

set(include_dirs 
    include
    )

set(requires
    esp-sr
    hardware_driver
    sr_ringbuf
    player
    )

idf_component_register(SRCS ${srcs}
                       INCLUDE_DIRS ${include_dirs}
                       REQUIRES ${requires})

add_definitions(-w)