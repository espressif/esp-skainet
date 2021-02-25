if(IDF_TARGET STREQUAL "esp32")
set(COMPONENT_SRCS
    esp_skainet_player.c
    ./esp_tts_wav/wav_encoder.c
    ./esp_tts_wav/wav_decoder.c
    )

set(COMPONENT_ADD_INCLUDEDIRS 
    ./
    ./esp_tts_wav
    )

set(COMPONENT_REQUIRES
    hardware_driver
    )
endif()

register_component()



