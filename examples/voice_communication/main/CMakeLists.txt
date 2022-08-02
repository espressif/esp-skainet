set(COMPONENT_SRCS
    main.c
    )

set(COMPONENT_REQUIRES
    esp-sr
    hardware_driver
    sr_ringbuf
    )

register_component()



