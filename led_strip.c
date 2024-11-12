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
#include "hardware/spi.h"
#include "hardware/dma.h"
#include "hardware/adc.h"
#include "pico/rand.h"
#include "apa102.h"
#include "hsl_to_rgb.h"


#define FRAME_RATE_MS 1000/24
#define LED_STRIP_LEN 60

APA102_LED *strip;
uint dma_spi_rx;
uint32_t iteration;
float filtered_h = 180.0, filtered_l = 500.0, filtered_s = 500.0;



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



void rx_dma_irq_handler() {
    // SPI DMA has dumped a complete APA102 LED image into the API's buffer. Send
    // it to the PIO, so we can see pretty lights.
    apa102_strip_update();

    // Reset the SPI RX DMA channel to write to the same buffer the PIO is currently 
    // reading from. Is this a race condition? You betcha. So just be careful how
    // fast you send SPI updates, mmkay?
    dma_channel_set_write_addr(dma_spi_rx, apa102_get_strip(), true);
}


int main() {
    stdio_init_all();
    pico_led_init();
    pico_set_led(false);

    adc_init();
    adc_gpio_init(26);

    strip = apa102_init(LED_STRIP_LEN);

    // Enable SPI at 1 MHz and connect to GPIOs
    spi_init(spi_default, 1000 * 1000);
    gpio_set_function(PICO_DEFAULT_SPI_RX_PIN, GPIO_FUNC_SPI);
    gpio_init(PICO_DEFAULT_SPI_CSN_PIN);
    gpio_set_function(PICO_DEFAULT_SPI_SCK_PIN, GPIO_FUNC_SPI);
    gpio_set_function(PICO_DEFAULT_SPI_TX_PIN, GPIO_FUNC_SPI);
    // Make the SPI pins available to picotool
    // bi_decl(bi_3pins_with_func(PICO_DEFAULT_SPI_RX_PIN, PICO_DEFAULT_SPI_TX_PIN, PICO_DEFAULT_SPI_SCK_PIN, GPIO_FUNC_SPI));
    // Make the CS pin available to picotool
    // bi_decl(bi_1pin_with_name(PICO_DEFAULT_SPI_CSN_PIN, "SPI CS"));

    // Grab some unused dma channels
    // const uint dma_tx = dma_claim_unused_channel(true);
    dma_spi_rx = dma_claim_unused_channel(true);

    // Force loopback for testing (I don't have an SPI device handy)
    // hw_set_bits(&spi_get_hw(spi_default)->cr1, SPI_SSPCR1_LBM_BITS);

    printf("Configure RX DMA\n");

    // We set the inbound DMA to transfer from the SPI receive FIFO to a memory buffer paced by the SPI RX FIFO DREQ
    // We configure the read address to remain unchanged for each element, but the write
    // address to increment (so data is written throughout the buffer)
    dma_channel_config rx_dma_config = dma_channel_get_default_config(dma_spi_rx);
    channel_config_set_transfer_data_size(&rx_dma_config, DMA_SIZE_32);
    channel_config_set_dreq(&rx_dma_config, spi_get_dreq(spi_default, false));
    channel_config_set_read_increment(&rx_dma_config, false);
    channel_config_set_write_increment(&rx_dma_config, true);
    dma_channel_set_irq0_enabled(dma_spi_rx, true);
    irq_set_exclusive_handler(DMA_IRQ_0, rx_dma_irq_handler);
    irq_set_enabled(DMA_IRQ_0, true);
    dma_channel_configure(dma_spi_rx, 
                          &rx_dma_config,
                          apa102_get_strip(),           // write to the APA102 buffer
                          &spi_get_hw(spi_default)->dr, // read address
                          apa102_get_buffer_size(),     // element count (each element is of size transfer_data_size)
                          true);

    while(true) {
        pico_set_led(true);
        sleep_ms(500);
        pico_set_led(false);
        sleep_ms(500);
    }
}
