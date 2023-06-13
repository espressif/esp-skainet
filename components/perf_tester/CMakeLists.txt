set(srcs
    perf_tester.c
    )

set(requires
    hardware_driver
    esp-sr
    player
    )

idf_component_register(
    SRCS ${srcs}
    INCLUDE_DIRS "."
    REQUIRES ${requires})

