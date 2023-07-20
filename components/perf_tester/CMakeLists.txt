set(srcs
    wn_perf_tester.c
    mn_perf_tester.c
    perf_tester_cmd.c
    )

set(requires
    hardware_driver
    esp-sr
    player
    console
    )

idf_component_register(
    SRCS ${srcs}
    INCLUDE_DIRS "."
    REQUIRES ${requires})

