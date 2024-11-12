
#include <stdio.h>
#include <math.h>
#include <string.h>
#include <stdlib.h>

#include "pico/stdlib.h"
#include "hardware/pio.h"
#include "hardware/dma.h"
#include "apa102.pio.h"
#include "apa102.h"

#define PIN_CLK 2
#define PIN_DIN 3
#define SERIAL_FREQ ((uint)(5 * 1000 * 1000))
#define MAX_STRIP_LENGTH 1000

#define APA102_START_FRAME ((APA102_LED)0);
#define APA102_END_FRAME ((APA102_LED)0xffffffff);

PIO apa102_pio = pio0;
uint apa102_sm = 0;
uint dma_pio_tx;
dma_channel_config apa102_dma_channel_config;


APA102_LED *apa102_strip = NULL;
uint16_t apa102_strip_length = 0;


APA102_LED *apa102_get_strip() {
    return apa102_strip;
}


// Number of actual LEDs in the strip
uint16_t apa102_get_strip_count() {
    return apa102_strip_length;
}


// The number of 32-bit words to transfer from SPI or to the PIO
uint16_t apa102_get_buffer_size() {
    return apa102_strip_length + 2;
}


void apa102_config_tx_dma() {
    dma_channel_configure(dma_pio_tx, 
                          &apa102_dma_channel_config,
                          &apa102_pio->txf[apa102_sm], // write to PIO TX FIFO
                          apa102_strip,                // read from LED buffer
                          apa102_get_buffer_size(),
                          false);                      // don't start yet
}


void apa102_strip_init(APA102_LED *strip, uint16_t strip_len) {
    // Start frame is 32 bits of all zeros
    memset(strip, 0, sizeof(APA102_LED));

    // Init strip to zero brighness black
    for(int i = 1; i < strip_len; i++) {
        strip[i].unused = 0x7;
        strip[i].brightness = 0;
        strip[i].red = 0;
        strip[i].green = 0;
        strip[i].blue = 0;
    }

    // End frame is 32 bits of all ones
    memset(&strip[strip_len + 1], 0xff, sizeof(APA102_LED));
}


void apa102_set_led(uint16_t led, uint8_t red, uint8_t green, uint8_t blue, uint8_t brightness) {
    if(led < apa102_strip_length) {
        apa102_strip[led + 1].unused = 7;
        apa102_strip[led + 1].brightness = brightness;
        apa102_strip[led + 1].red = red;
        apa102_strip[led + 1].green = green;
        apa102_strip[led + 1].blue = blue;
    }
}


void apa102_strip_update() {    
    apa102_config_tx_dma();
    dma_start_channel_mask(1u << dma_pio_tx);
}


APA102_LED *apa102_init(uint16_t strip_len) {
    if(strip_len <= MAX_STRIP_LENGTH) {
        // Allocate memory for strip length plus start and end frames
        apa102_strip = (APA102_LED *)malloc((strip_len + 2) * sizeof(APA102_LED));
        apa102_strip_length = strip_len;
        apa102_strip_init(apa102_strip, apa102_strip_length);

        uint offset = pio_add_program(apa102_pio, &apa102_mini_program);
        apa102_mini_program_init(apa102_pio, apa102_sm, offset, SERIAL_FREQ, PIN_CLK, PIN_DIN);

        dma_pio_tx = dma_claim_unused_channel(true);
        apa102_dma_channel_config = dma_channel_get_default_config(dma_pio_tx);
        // channel_config_set_bswap(&apa102_dma_channel_config, true);
        channel_config_set_transfer_data_size(&apa102_dma_channel_config, DMA_SIZE_32);
        channel_config_set_dreq(&apa102_dma_channel_config, pio_get_dreq(apa102_pio, apa102_sm, true));
    }

    return apa102_strip;
}


char *apa102_led_to_s(APA102_LED *led, char *buffer) {
    sprintf(buffer, "%02x %02x %02x %02d", led->red, led->green, led->blue, led->brightness);

    return buffer;
}

void apa102_log_strip() {
    char buffer[25];

    for(int n = 0; n < apa102_strip_length + 2; n ++) {
        apa102_led_to_s(&apa102_strip[n], buffer);
        printf("%s\n", buffer);
    }
    printf("\n");
}