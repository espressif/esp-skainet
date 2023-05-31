set(srcs
    esp_skainet_player.c
    ./esp_tts_wav/wav_encoder.c
    ./esp_tts_wav/wav_decoder.c
    )

idf_component_register(
    SRCS ${srcs}
    INCLUDE_DIRS
        "."
        "./esp_tts_wav"
    REQUIRES
        hardware_driver)

