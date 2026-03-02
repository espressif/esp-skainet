set(srcs main.c)

set(requires
    hardware_driver
    player
    )

idf_component_register(SRCS ${srcs}
                       REQUIRES ${requires})
