set(requires
    hardware_driver
    sr_ringbuf
    )

idf_component_register(SRCS main.c
                       REQUIRES ${requires})

