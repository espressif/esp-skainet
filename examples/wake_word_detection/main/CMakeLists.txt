set(requires
    hardware_driver
    player
    )

idf_component_register(SRCS main.c
                       REQUIRES ${requires})

