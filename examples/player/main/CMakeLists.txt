set(srcs main.c)

set(requires
    hardware_driver
    player
    console
    )

idf_component_register(SRCS ${srcs}
                       REQUIRES ${requires})
