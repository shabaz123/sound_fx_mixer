cmake_minimum_required(VERSION 3.12)
include(pico_sdk_import.cmake)
include(pico_extras_import.cmake)
# set(PICO_BOARD pico_w)
project(sound_fx_mixer C CXX)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
pico_sdk_init()

add_executable(sound_fx_mixer
        main.c
        cat1.c
        cat2.c
        cow1.c
        cow2.c
        cow3.c
        dog1.c
        dog2.c
        goat1.c
        goat2.c
        hen1.c
        hen2.c
        pig1.c
        pig2.c
        rooster1.c
        sheep1.c
        sheep2.c
        snake1.c
        snake2.c
        snake3.c
        wolf1.c
        wolf2.c
        )

target_link_libraries(sound_fx_mixer PRIVATE
        pico_stdlib
        pico_audio_i2s
        )

target_compile_definitions(sound_fx_mixer PRIVATE
        # compile time configuration of I2S
        PICO_AUDIO_I2S_MONO_INPUT=1
        #define for our example code
        USE_AUDIO_I2S=1
#            PICO_AUDIO_I2S_DATA_PIN=22
#            PICO_AUDIO_I2S_CLOCK_PIN_BASE=23
#            PICO_DEFAULT_UART=0
#            PICO_DEFAULT_UART_TX_PIN=28
#            PICO_DEFAULT_UART_RX_PIN=29
        )
# enable usb output, disable uart output
pico_enable_stdio_usb(sound_fx_mixer 1)
pico_enable_stdio_uart(sound_fx_mixer 0)
# create map/bin/hex file etc.
pico_add_extra_outputs(sound_fx_mixer)

