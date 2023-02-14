set(srcs
    esp_skainet_player.c
    ./esp_tts_wav/wav_encoder.c
    ./esp_tts_wav/wav_decoder.c
    )

set(include_dirs 
    ./
    ./esp_tts_wav
    )

idf_component_register(SRCS ${srcs}
                       INCLUDE_DIRS ${include_dirs}
                       REQUIRES hardware_driver)
