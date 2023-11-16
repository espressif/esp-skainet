set(srcs
    multinet_main.c
   )

set(requires
    perf_tester
    )

idf_component_register(SRCS ${srcs}
                       INCLUDE_DIRS .
                       REQUIRES ${requires})

