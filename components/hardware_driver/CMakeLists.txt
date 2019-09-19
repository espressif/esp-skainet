set(COMPONENT_SRCDIRS 
    ./MediaHal
    ./MediaHal/Codec
    ./SystemSal
    ./i2c_bus
    ./src
    )

set(COMPONENT_ADD_INCLUDEDIRS 
    ./MediaHal/Board
    ./MediaHal/Codec
    ./MediaHal/
    ./SystemSal
    ./i2c_bus
    ./src
    ./userconfig
    )


register_component()


