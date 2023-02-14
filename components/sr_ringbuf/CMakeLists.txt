set(srcs
    ringbuf.c
    EspAudioAlloc.c
    lock.c
    ringbuf.c
    )

set(include_dirs 
    ./
    )

idf_component_register(SRCS ${srcs}
                       INCLUDE_DIRS ${include_dirs})
