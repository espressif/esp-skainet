set(COMPONENT_SRCS
    esp_skainet_player.c
    ./esp_tts_wav/wav_encoder.c
    ./esp_tts_wav/wav_decoder.c
    )

set(COMPONENT_ADD_INCLUDEDIRS 
    ./
    ./esp_tts_wav
    )

register_component()



