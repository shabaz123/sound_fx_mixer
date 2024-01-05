/********************************************************************************
 * sound_fx_mixer
 * main.c
 * Note: this code uses snippets from pico audio example from raspberrypi.org
 * rev 1 - shabaz - Jan 2024
 ********************************************************************************/

#include <stdio.h>
#include <math.h>
#include "hardware/gpio.h"

// only I2S mode has been attempted for this project
#ifndef USE_AUDIO_I2S
#define USE_AUDIO_I2S 1
#endif
// Pins 13,14,15 are used for I2S
#define PICO_AUDIO_I2S_DATA_PIN 13
#define PICO_AUDIO_I2S_CLOCK_PIN_BASE 14
#if PICO_ON_DEVICE
#include "hardware/clocks.h"
#include "hardware/structs/clocks.h"
#endif
#include "pico/stdlib.h"
#if USE_AUDIO_I2S
#include "pico/audio_i2s.h"
#if PICO_ON_DEVICE
#include "pico/binary_info.h"
bi_decl(bi_3pins_with_names(PICO_AUDIO_I2S_DATA_PIN, "I2S DIN", PICO_AUDIO_I2S_CLOCK_PIN_BASE, "I2S BCK", PICO_AUDIO_I2S_CLOCK_PIN_BASE+1, "I2S LRCK"));
#endif
#elif USE_AUDIO_PWM
#include "pico/audio_pwm.h"
#endif

// defines
#define COUNT_OF(x) ((sizeof(x)/sizeof(0[x])) / ((size_t)(!(sizeof(x) % sizeof(0[x])))))
// number of concurrent sounds that can be played out
#define NUM_VOICES 8
#define LED_PIN 12
#define LED_ON gpio_put(LED_PIN, 1)
#define LED_OFF gpio_put(LED_PIN, 0)

/********* extern declarations for all the sound arrays *********/
extern const int16_t cat1[18472];
extern const int16_t cat2[15190];
extern const int16_t cow1[36258];
extern const int16_t cow2[24021];
extern const int16_t cow3[41626];
extern const int16_t dog1[4497];
extern const int16_t dog2[22168];
extern const int16_t goat1[46465];
extern const int16_t goat2[20310];
extern const int16_t hen1[28470];
extern const int16_t hen2[10615];
extern const int16_t pig1[7364];
extern const int16_t pig2[16359];
extern const int16_t rooster1[59365];
extern const int16_t sheep1[32440];
extern const int16_t sheep2[27054];
extern const int16_t snake1[105514];
extern const int16_t snake2[61650];
extern const int16_t snake3[31786];
extern const int16_t wolf1[113098];
extern const int16_t wolf2[46860];

// array of pointers to the sound arrays
const int16_t *sound_array[21] = {
        cat1,     /*   0   */
        cat2,     /*   1   */
        cow1,     /*   2   */
        cow2,     /*   3   */
        cow3,     /*   4   */
        dog1,     /*   5   */
        dog2,     /*   6   */
        goat1,    /*   7   */
        goat2,    /*   8   */
        hen1,     /*   9   */
        hen2,     /*  10   */
        pig1,     /*  11   */
        pig2,     /*  12   */
        rooster1, /*  13   */
        sheep1,   /*  14   */
        sheep2,   /*  15   */
        snake1,   /*  16   */
        snake2,   /*  17   */
        snake3,   /*  18   */
        wolf1,    /*  19   */
        wolf2     /*  20   */
};

// definitions for convenience
#define CAT1 0
#define CAT2 1
#define COW1 2
#define COW2 3
#define COW3 4
#define DOG1 5
#define DOG2 6
#define GOAT1 7
#define GOAT2 8
#define HEN1 9
#define HEN2 10
#define PIG1 11
#define PIG2 12
#define ROOSTER1 13
#define SHEEP1 14
#define SHEEP2 15
#define SNAKE1 16
#define SNAKE2 17
#define SNAKE3 18
#define WOLF1 19
#define WOLF2 20

// array of lengths of the sound arrays
const int sound_array_len[21] = {
        COUNT_OF(cat1), COUNT_OF(cat2), COUNT_OF(cow1), COUNT_OF(cow2), COUNT_OF(cow3),
        COUNT_OF(dog1), COUNT_OF(dog2), COUNT_OF(goat1), COUNT_OF(goat2), COUNT_OF(hen1),
        COUNT_OF(hen2), COUNT_OF(pig1), COUNT_OF(pig2), COUNT_OF(rooster1), COUNT_OF(sheep1),
        COUNT_OF(sheep2), COUNT_OF(snake1), COUNT_OF(snake2), COUNT_OF(snake3), COUNT_OF(wolf1),
        COUNT_OF(wolf2)
};

// map the buttons to particular sounds
// (it is OK to have multiple buttons mapped to the same sound if desired)
const int button_sound_map[8] = {CAT1, COW1, DOG1, GOAT1, HEN1, ROOSTER1, SNAKE1, WOLF1};

// button array
// GPIO numbers
const uint button_gpio[8] = {0, 1, 2, 3, 4, 5, 6, 7};

#define SINE_WAVE_TABLE_LEN 2048
#define SAMPLES_PER_BUFFER 256

// ******** global variables ********
uint8_t last_button_state = 0;
static int16_t sine_wave_table[SINE_WAVE_TABLE_LEN];
// the voice_sound_index array stores a value from 0 to 20, which is the sound array index
// a value of -1 means the voice is not playing.
int voice_sound_index[NUM_VOICES];
uint32_t voice_sound_pos[NUM_VOICES]; // stores the current position in the sound array

// init_audio() function
struct audio_buffer_pool *init_audio() {
    static audio_format_t audio_format = {
            .format = AUDIO_BUFFER_FORMAT_PCM_S16,
            .sample_freq = 24000,
            .channel_count = 1,
    };
    static struct audio_buffer_format producer_format = {
            .format = &audio_format,
            .sample_stride = 2
    };
    struct audio_buffer_pool *producer_pool = audio_new_producer_pool(&producer_format, 3,
                                                                      SAMPLES_PER_BUFFER); // todo correct size
    bool __unused ok;
    const struct audio_format *output_format;
#if USE_AUDIO_I2S
    struct audio_i2s_config config = {
            .data_pin = PICO_AUDIO_I2S_DATA_PIN,
            .clock_pin_base = PICO_AUDIO_I2S_CLOCK_PIN_BASE,
            .dma_channel = 0,
            .pio_sm = 0,
    };
    output_format = audio_i2s_setup(&audio_format, &config);
    if (!output_format) {
        panic("PicoAudio: Unable to open audio device.\n");
    }
    ok = audio_i2s_connect(producer_pool);
    assert(ok);
    audio_i2s_set_enabled(true);
#elif USE_AUDIO_PWM
    output_format = audio_pwm_setup(&audio_format, -1, &default_mono_channel_config);
    if (!output_format) {
        panic("PicoAudio: Unable to open audio device.\n");
    }
    ok = audio_pwm_default_connect(producer_pool, false);
    assert(ok);
    audio_pwm_set_enabled(true);
#endif
    return producer_pool;
}

void board_init(void) {
    int i;
    // setup buttons
    for (i=0; i<8; i++) {
        gpio_init(button_gpio[i]);
        gpio_set_dir(button_gpio[i], GPIO_IN);
        gpio_pull_up(button_gpio[i]); // pullup enabled
    }
    // setup LED
    gpio_init(LED_PIN);
    gpio_set_dir(LED_PIN, GPIO_OUT);
    LED_OFF;
    // initialize arrays
    for (i=0; i<NUM_VOICES; i++) {
        voice_sound_index[i] = -1;
        voice_sound_pos[i] = 0;
    }
    sleep_ms(100); // let any attached hardware settle
}

// returns a byte with the current button level (1 = pressed)
uint8_t get_button_level(void) {
    uint8_t button_level = 0;
    int i;
    for (i=0; i<8; i++) {
        if (gpio_get(button_gpio[i]) == false) { // button is pressed!
            button_level |= (1<<i);
        }
    }
    if (button_level != 0) {
        // printf("get_button_level: %02x, last_button_state: %02x\n", button_level, last_button_state);
    }
    return button_level;
}

// returns a byte with only new pressed buttons (1 = pressed)
uint8_t get_new_button_press(void) {
    uint8_t button_state = get_button_level();
    uint8_t button_press = button_state & ~last_button_state;
    last_button_state = button_state;
    if (button_state != 0) {
        // printf("get_new_button_press: %02x\n", button_press);
    }
    return button_press;
}

int main() {
    uint8_t buttons;
    int j;
#if PICO_ON_DEVICE
#if USE_AUDIO_PWM
    set_sys_clock_48mhz();
#endif
#endif
    stdio_init_all();
    board_init();

    for (int i = 0; i < SINE_WAVE_TABLE_LEN; i++) {
        sine_wave_table[i] = 32767 * cosf(i * 2 * (float) (M_PI / SINE_WAVE_TABLE_LEN));
    }

    struct audio_buffer_pool *ap = init_audio();
    uint32_t step = 0x200000;
    uint32_t pos = 0;
    uint32_t pos_max = 0x10000 * SINE_WAVE_TABLE_LEN;
    uint vol = 128;
    while (true) { // forever loop
#if USE_AUDIO_PWM
        enum audio_correction_mode m = audio_pwm_get_correction_mode();
#endif
        LED_OFF;
        // check if any buttons have been pressed
        buttons = get_new_button_press();
        if (buttons) {
            // a button has been pressed
            for (j=0; j<8; j++) {
                if (buttons & (1<<j)) {
                    // a button has been pressed
                    LED_ON;
                    // find a free voice
                    for (int k=0; k<NUM_VOICES; k++) {
                        if (voice_sound_index[k] == -1) {
                            // found a free voice
                            voice_sound_index[k] = button_sound_map[j];
                            voice_sound_pos[k] = 0;
                            break;
                        }
                    }
                }
            }
        }

        // fill the audio sample buffer
        struct audio_buffer *buffer = take_audio_buffer(ap, true);
        int16_t *samples = (int16_t *) buffer->buffer->bytes;
        for (uint i = 0; i < buffer->max_sample_count; i++) {
            // create a sample by adding all the voices together
            samples[i] = 0;
            for (j=0; j<NUM_VOICES; j++) {
                if (voice_sound_index[j] != -1) {
                    // add one eighth of the amplitude to the sample
                    samples[i] += (sound_array[voice_sound_index[j]][voice_sound_pos[j]]) >> 3;
                    voice_sound_pos[j]++;
                    // if the sound has finished, set the index to -1
                    if (voice_sound_pos[j] >= sound_array_len[voice_sound_index[j]]) {
                        voice_sound_index[j] = -1;
                    }
                }
            }
        }
        buffer->sample_count = buffer->max_sample_count;
        give_audio_buffer(ap, buffer);
    }
    puts("\n");
    return 0;
}
