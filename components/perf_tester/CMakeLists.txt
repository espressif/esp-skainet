set(srcs
    wn_perf_tester.c
    mn_perf_tester.c
    perf_tester_cmd.c
    )

set(requires
    hardware_driver
    player
    console
    esp-sr
    )

idf_component_register(
    SRCS ${srcs}
    INCLUDE_DIRS "."
    REQUIRES ${requires})

