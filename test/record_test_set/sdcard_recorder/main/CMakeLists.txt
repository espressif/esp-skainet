set(requires
    hardware_driver
    sr_ringbuf
    player
    )

idf_component_register(SRCS main.c uart.c
                       INCLUDE_DIRS .
                       REQUIRES ${requires})

add_definitions(-w)