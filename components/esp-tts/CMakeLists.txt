set(COMPONENT_ADD_INCLUDEDIRS
    ./esp_chinese_tts/include
    )


register_component()

target_link_libraries(${COMPONENT_TARGET} INTERFACE "-L ${CMAKE_CURRENT_SOURCE_DIR/esp_chinese_tts}")
target_link_libraries(${COMPONENT_TARGET} INTERFACE speech_synthesis 
                                                    voice_set_xiaole )

