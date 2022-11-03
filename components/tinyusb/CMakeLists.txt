idf_component_register(SRCS "usb_descriptors.c"
                    INCLUDE_DIRS "." "src"
                    REQUIRES freertos soc)

target_sources(${COMPONENT_LIB} PRIVATE
  "src/tusb.c"
  "src/common/tusb_fifo.c"
  "src/device/usbd.c"
  "src/device/usbd_control.c"
  "src/class/audio/audio_device.c"
  "src/portable/synopsys/dwc2/dcd_dwc2.c"
  #"src/portable/espressif/esp32sx/dcd_esp32sx.c"
)

target_compile_options(${COMPONENT_LIB} PRIVATE "-Wno-format")

target_compile_options(${COMPONENT_LIB} PUBLIC
  "-DCFG_TUSB_OS=OPT_OS_FREERTOS"
  #"-DCFG_EXAMPLE_VIDEO_READONLY"
)

if(IDF_TARGET STREQUAL "esp32s2")
  target_compile_options(${COMPONENT_LIB} PUBLIC
    "-DCFG_TUSB_MCU=OPT_MCU_ESP32S2")
elseif(IDF_TARGET STREQUAL "esp32s3")
  target_compile_options(${COMPONENT_LIB} PUBLIC
    "-DCFG_TUSB_MCU=OPT_MCU_ESP32S3")
else()
  message(FATAL_ERROR "${IDF_TARGET} not supported by tinyusb")
endif()