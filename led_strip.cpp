/**
 * Copyright (c) 2020 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "pico/stdlib.h"
#include "pico/binary_info.h"
#include "hardware/dma.h"
#include "hardware/clocks.h"
#include "hardware/pio.h"
#include "hardware/sync.h"
#include "pico/rand.h"
#include "apa102.hpp"
#include "spi_rx.pio.h"
#include "hsl_to_rgb.h"
#include "pico/critical_section.h"



#define FRAME_RATE_MS 1000/24
#define LED_STRIP_LEN 60
#define LED_CONFIG_WORD_LEN 4
#define SPI_RX_BUFFER_LEN ((LED_STRIP_LEN) * LED_CONFIG_WORD_LEN)

uint32_t iteration;

uint8_t spi_rx_buffer_1[SPI_RX_BUFFER_LEN], spi_rx_buffer_2[SPI_RX_BUFFER_LEN];
uint8_t *spi_rx_active_buffer = spi_rx_buffer_1;
uint8_t *spi_rx_standby_buffer = spi_rx_buffer_2;
static volatile uint8_t *spi_rx_buffer = NULL;

int spi_rx_dma_channel = -1;
bool spi_rx_dma_complete = false;

PIO spi_rx_pio = pio1;
int spi_rx_sm = 0;


int pico_led_init(void) {
    gpio_init(PICO_DEFAULT_LED_PIN);
    gpio_set_dir(PICO_DEFAULT_LED_PIN, GPIO_OUT);
    return PICO_OK;
}


void pico_set_led(bool led_on) {
    gpio_put(PICO_DEFAULT_LED_PIN, led_on);
}


void printbuf(uint8_t buf[], size_t len) {
    size_t i;
    for (i = 0; i < len; ++i) {
        if (i % 16 == 15)
            printf("%02x\n", buf[i]);
        else
            printf("%02x ", buf[i]);
    }

    // append trailing newline if there isn't one
    if (i % 16) {
        putchar('\n');
    }
}


void swap_rx_buffers() {
    uint8_t *temp;

    uint32_t status = save_and_disable_interrupts();
    
    temp = spi_rx_active_buffer;
    spi_rx_active_buffer = spi_rx_standby_buffer;
    spi_rx_standby_buffer = temp;

    restore_interrupts(status);
}


void rx_dma_irq_handler() {
    dma_channel_acknowledge_irq0(spi_rx_dma_channel);
    __dmb();
    spi_rx_buffer = spi_rx_active_buffer;
    __dmb();
    swap_rx_buffers();
    dma_channel_set_write_addr(spi_rx_dma_channel, spi_rx_active_buffer, true);
    __sev();
}


void spi_rx_init() {
#if 0
    uint offset = pio_add_program(spi_rx_pio, &spi_rx_program);

    float spi_clock = 1000000;
    float system_clock = clock_get_hz(clk_peri);
    float pio_divider = system_clock / (spi_clock * 4);

    pio_spi_rx_init(spi_rx_pio, 
                    spi_rx_sm, 
                    offset,
                    8,
                    pio_divider,
                    PICO_DEFAULT_SPI_SCK_PIN,
                    PICO_DEFAULT_SPI_RX_PIN);

    spi_rx_dma_channel = dma_claim_unused_channel(true);
    
    // Configure the DMA channel
    dma_channel_config spi_rx_dma_config = dma_channel_get_default_config(spi_rx_dma_channel);
    channel_config_set_transfer_data_size(&spi_rx_dma_config, DMA_SIZE_8);
    channel_config_set_read_increment(&spi_rx_dma_config, false);
    channel_config_set_write_increment(&spi_rx_dma_config, true);
    channel_config_set_dreq(&spi_rx_dma_config, pio_get_dreq(spi_rx_pio, spi_rx_sm, false));

    // Setup interrupt handler
    dma_channel_set_irq0_enabled(spi_rx_dma_channel, true);
    irq_set_exclusive_handler(DMA_IRQ_0, rx_dma_irq_handler);
    irq_set_enabled(DMA_IRQ_0, true);

    // Setup the channel
    dma_channel_configure(
        spi_rx_dma_channel,
        &spi_rx_dma_config,
        spi_rx_active_buffer,         // Write to buffer
        &spi_rx_pio->rxf[spi_rx_sm],  // Read from PIO RX FIFO
        SPI_RX_BUFFER_LEN,            // Number of bytes
        true                          // Start immediately
    );
#endif
}


int main() {
    stdio_init_all();
    pico_led_init();
    sleep_ms(2000); // wait for serial port for some reason
    pico_set_led(true);

    printf("led_strip\n");

    // spi_rx_init();
    iteration = 0;

    // APA102 strip1(1, 2, 3, pio0, 0);
    APA102 strip2(3, 4, 5, pio0, 1);

    // strip1.set_led(0, 1, 1, 1, 7);

    uint8_t red = 0, green = 0, blue = 0;
    strip2.set_led(0, red, 0, 0, 7);
    strip2.set_led(1, 0, green, 0, 7);
    strip2.set_led(2, 0, 0, blue, 7);

    // __sev();
    // __wfe();
    while(true) {
        iteration++;
        sleep_ms(10);
        pico_set_led(true);
        // strip1.update_strip();
        red++;
        green++;
        blue++;
        strip2.set_led(0, red, 0, 0, 7);
        strip2.set_led(1, 0, green, 0, 7);
        strip2.set_led(2, 0, 0, blue, 7);

        strip2.update_strip();
        sleep_ms(10);
        // printf("ITERATION %d\n", iteration);
        pico_set_led(false);
#if 0
        __dmb();
        if(spi_rx_buffer) {
            pico_set_led(true);
            for(int n = 0; n < LED_STRIP_LEN; n++) {
                uint8_t brightness, red, green, blue;
                uint8_t *led_config;
                led_config = &spi_rx_buffer[n * 4];
                brightness = led_config[0];
                red = led_config[1];
                green = led_config[2];
                blue = led_config[3];
                apa102_set_led(n, red, green, blue, brightness);
            }
            apa102_strip_update();
            __dmb();
            spi_rx_buffer = NULL;
            __dmb();
            pico_set_led(false);
        }
        __wfe();
#endif
    }
}
