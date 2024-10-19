
#include <stdio.h>
#include <math.h>

#include "pico/stdlib.h"
#include "hardware/pio.h"
#include "apa102.pio.h"
#include "apa102.h"

#define PIN_CLK 2
#define PIN_DIN 3
#define SERIAL_FREQ (5 * 1000 * 1000)

void apa102_start_frame(PIO pio, uint sm) {
    pio_sm_put_blocking(pio, sm, 0u);
}

void ap102_end_frame(PIO pio, uint sm) {
    pio_sm_put_blocking(pio, sm, ~0u);
}

void ap102_write(PIO pio, uint sm, uint8_t brightness, uint8_t r, uint8_t g, uint8_t b) {
    pio_sm_put_blocking(pio, sm,
                        0x7 << 29 |                   // magic
                        (brightness & 0x1f) << 24 |   // global brightness parameter
                        (uint32_t) b << 16 |
                        (uint32_t) g << 8 |
                        (uint32_t) r << 0
    );
}


PIO apa102_pio = pio0;
uint apa102_sm = 0;

void apa102_init() {
    uint offset = pio_add_program(apa102_pio, &apa102_mini_program);
    apa102_mini_program_init(apa102_pio, apa102_sm, offset, SERIAL_FREQ, PIN_CLK, PIN_DIN);
}


void apa102_strip_init(APA102_LED *strip, uint16_t strip_len) {
    for(int i = 0; i < strip_len; i++) {
        strip[i].unused = 0x7;
        strip[i].brightness = 15;
        strip[i].red = 0;
        strip[i].green = 0;
        strip[i].blue = 0;
    }
}


void apa102_strip_update(APA102_LED *strip, uint16_t strip_len) {
    apa102_start_frame(apa102_pio, apa102_sm);
    
    for(int n = 0; n < strip_len; n++) {
        ap102_write(apa102_pio, apa102_sm, 
                    strip[n].brightness,
                    strip[n].red,
                    strip[n].green,
                    strip[n].blue);
    }
    
    ap102_end_frame(apa102_pio, apa102_sm);
}
