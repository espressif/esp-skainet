set(srcs
    main.c
    )

set(requires
    hardware_driver
    )

idf_component_register(SRCS ${srcs}
                       INCLUDE_DIRS include
                       REQUIRES ${requires})

component_compile_options(-w)
