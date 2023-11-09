set(srcs
    main.c
    tts_urat.c
    )

set(include_dirs 
    include
    )

set(requires
    hardware_driver
    sr_ringbuf
    player
    )

idf_component_register(SRCS ${srcs}
                       INCLUDE_DIRS ${include_dirs}
                       REQUIRES ${requires})

add_definitions(-w)

# Determine whether esp-sr is fetched from component registry or from local path
idf_build_get_property(build_components BUILD_COMPONENTS)
if(esp-sr IN_LIST build_components)
    set(esp-sr_name esp-sr) # Local component
else()
    set(esp-sr_name espressif__esp-sr) # Managed component
endif()

# Get path to voice data file
idf_component_get_property(esp-sr_path ${esp-sr_name} COMPONENT_DIR)
set(voice_data_image ${esp-sr_path}/esp-tts/esp_tts_chinese/esp_tts_voice_data_xiaoxin_small.dat)
add_custom_target(voice_data ALL DEPENDS ${voice_data_image})
add_dependencies(flash voice_data)

partition_table_get_partition_info(size "--partition-name voice_data" "size")
partition_table_get_partition_info(offset "--partition-name voice_data" "offset")

if("${size}" AND "${offset}")
    esptool_py_flash_to_partition(flash "voice_data" "${voice_data_image}")
else()
    set(message "Failed to find model in partition table file"
                "Please add a line(Name=voice_data, Type=data, Size=3890K) to the partition file.")
endif()