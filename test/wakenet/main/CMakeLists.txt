set(srcs
    wakenet_main.c
   )

set(requires
    esp-sr
    perf_tester
    )

idf_component_register(SRCS ${srcs}
                       INCLUDE_DIRS .
                       REQUIRES ${requires})

