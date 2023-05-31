if(IDF_TARGET STREQUAL "esp32s3")
    set(component_srcs "src/led_strip_rmt_ws2812.c")

    idf_component_register(SRCS "${component_srcs}"
                           INCLUDE_DIRS "include"
                           PRIV_INCLUDE_DIRS ""
                           PRIV_REQUIRES "driver"
                           REQUIRES "")
    
    component_compile_options(-w)
    target_compile_options(${COMPONENT_LIB} PRIVATE "-Wno-format")
endif()
