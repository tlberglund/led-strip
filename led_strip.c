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
// #include "hardware/adc.h"
#include "hardware/pio.h"
#include "pico/rand.h"
#include "apa102.h"
#include "spi_rx.pio.h"
#include "hsl_to_rgb.h"


#define FRAME_RATE_MS 1000/24
#define LED_STRIP_LEN 60
#define LED_CONFIG_WORD_LEN 4
#define SPI_RX_BUFFER_LEN ((LED_STRIP_LEN + 1) * LED_CONFIG_WORD_LEN)
#define LED_CONFIG_RED(buf, n) (*((((uint8_t *)buf) + (n * LED_CONFIG_WORD_LEN)) + 1))
#define LED_CONFIG_GREEN(buf, n) (*((((uint8_t *)buf) + (n * LED_CONFIG_WORD_LEN)) + 2))
#define LED_CONFIG_BLUE(buf, n) (*((((uint8_t *)buf) + (n * LED_CONFIG_WORD_LEN)) + 3))
#define LED_CONFIG_BRIGHTNESS(buf, n) (*((((uint8_t *)buf) + (n * LED_CONFIG_WORD_LEN)) + 0))

APA102_LED *strip;
uint32_t iteration;
float filtered_h = 180.0, filtered_l = 500.0, filtered_s = 500.0;
uint8_t spi_rx_buffer[SPI_RX_BUFFER_LEN];

int spi_rx_dma_channel = -1;
bool spi_rx_dma_complete = false;

PIO apa102_pio = pio0;
int apa102_sm = 0;
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


bool examine_rx_buffer = false;
void rx_dma_irq_handler() {
    dma_channel_acknowledge_irq0(spi_rx_dma_channel);
    // Reset the SPI RX DMA channel to the one singular solitary buffer
    dma_channel_set_write_addr(spi_rx_dma_channel, spi_rx_buffer, true);
    examine_rx_buffer = true;
}


void spi_rx_init() {
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
        spi_rx_buffer,                // Write to buffer
        &spi_rx_pio->rxf[spi_rx_sm],  // Read from PIO RX FIFO
        60 * 4,                       // Number of bytes
        true                          // Start immediately
    );

}

int main() {
    stdio_init_all();
    pico_led_init();
    sleep_ms(500);
    pico_set_led(true);

    // gpio_set_dir(15, GPIO_OUT);
    // gpio_put(15, 0);

    // adc_init();
    // adc_gpio_init(26);

    spi_rx_init();
    iteration = 0;
    strip = apa102_init(apa102_pio, apa102_sm, LED_STRIP_LEN);

    printf("strip=%08x\n", strip);

    while(true) {
        if(examine_rx_buffer) {
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
            examine_rx_buffer = false;
        }
        pico_set_led(true);
        sleep_ms(50);
        pico_set_led(false);
        sleep_ms(50);
        iteration++;
    }

}
