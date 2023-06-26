idf_component_register(SRCS main.c src/usb_mic_recorder.c src/usb_descriptors.c
                       INCLUDE_DIRS include)

idf_component_get_property(tusb_lib leeebo__tinyusb_src COMPONENT_LIB)
cmake_policy(SET CMP0079 NEW)
target_link_libraries(${tusb_lib} PRIVATE ${COMPONENT_LIB})