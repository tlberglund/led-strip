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
#define SPI_RX_BUFFER_LEN 1000
#define LED_CONFIG_WORD_LEN 4
#define LED_CONFIG_RED(buf, n) (*((((uint8_t *)buf) + (n * LED_CONFIG_WORD_LEN)) + 1))
#define LED_CONFIG_GREEN(buf, n) (*((((uint8_t *)buf) + (n * LED_CONFIG_WORD_LEN)) + 2))
#define LED_CONFIG_BLUE(buf, n) (*((((uint8_t *)buf) + (n * LED_CONFIG_WORD_LEN)) + 3))
#define LED_CONFIG_BRIGHTNESS(buf, n) (*((((uint8_t *)buf) + (n * LED_CONFIG_WORD_LEN)) + 0))

APA102_LED *strip;
uint dma_spi_rx;
uint32_t iteration;
float filtered_h = 180.0, filtered_l = 500.0, filtered_s = 500.0;
uint8_t spi_rx_buffer[SPI_RX_BUFFER_LEN];


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
    gpio_put(15, 1);
    examine_rx_buffer = true;
    // printf("SPI DMA IRQ");
    // printbuf(spi_rx_buffer, 4);
    // printf("%02x %02x %02x %02x\n", spi_rx_buffer[0],spi_rx_buffer[1],spi_rx_buffer[2],spi_rx_buffer[3]);

#if 0
    // Convert SPI buffer of abstract LED configs into APA102-packed data
    for(int n = 0; n < LED_STRIP_LEN; n ++) {
        apa102_set_led(n,
                       LED_CONFIG_RED(spi_rx_buffer, n),
                       LED_CONFIG_GREEN(spi_rx_buffer, n),
                       LED_CONFIG_BLUE(spi_rx_buffer, n),
                       LED_CONFIG_BRIGHTNESS(spi_rx_buffer, n));
    }

    // Actually program the LEDs
    apa102_strip_update();
#endif
    // Reset the SPI RX DMA channel to the one singular solitary buffer
    dma_channel_set_write_addr(dma_spi_rx, spi_rx_buffer, true);
    gpio_put(15, 0);
}


int main() {
    stdio_init_all();
    pico_led_init();
    pico_set_led(true);
    sleep_ms(2000); // wait for USB serial
    pico_set_led(false);
    gpio_set_dir(15, GPIO_OUT);
    gpio_put(15, 0);
#if 0
    adc_init();
    adc_gpio_init(26);

    iteration = 0;
    strip = apa102_init(LED_STRIP_LEN);
#endif 

    // Enable SPI at 1 MHz and connect to GPIOs
    spi_init(spi0, 1000 * 1000);
    spi_set_slave(spi0, true);
    spi_set_format(spi0, 8, 0, 0, SPI_MSB_FIRST);
    gpio_set_function(PICO_DEFAULT_SPI_RX_PIN, GPIO_FUNC_SPI);
    gpio_set_function(PICO_DEFAULT_SPI_SCK_PIN, GPIO_FUNC_SPI);
    gpio_set_function(PICO_DEFAULT_SPI_TX_PIN, GPIO_FUNC_SPI);
    gpio_set_function(PICO_DEFAULT_SPI_CSN_PIN, GPIO_FUNC_SPI);

#if 0
    // Grab some unused dma channels
    // const uint dma_tx = dma_claim_unused_channel(true);
    dma_spi_rx = dma_claim_unused_channel(true);

    // Force loopback for testing (I don't have an SPI device handy)
    // hw_set_bits(&spi_get_hw(spi0)->cr1, SPI_SSPCR1_LBM_BITS);

    printf("Configure RX DMA\n");

    // We set the inbound DMA to transfer from the SPI receive FIFO to a memory buffer paced by the SPI RX FIFO DREQ
    // We configure the read address to remain unchanged for each element, but the write
    // address to increment (so data is written throughout the buffer)
    dma_channel_config rx_dma_config = dma_channel_get_default_config(dma_spi_rx);
    channel_config_set_transfer_data_size(&rx_dma_config, DMA_SIZE_8);
    channel_config_set_dreq(&rx_dma_config, spi_get_dreq(spi0, false));
    channel_config_set_read_increment(&rx_dma_config, false);
    channel_config_set_write_increment(&rx_dma_config, true);
    dma_channel_set_irq0_enabled(dma_spi_rx, true);
    irq_set_exclusive_handler(DMA_IRQ_0, rx_dma_irq_handler);
    irq_set_enabled(DMA_IRQ_0, true);
    dma_channel_configure(dma_spi_rx, 
                          &rx_dma_config,
                          spi_rx_buffer,                       // write to the APA102 buffer
                          &spi_get_hw(spi0)->dr,               // read address
                          4,
                        //   LED_STRIP_LEN * LED_CONFIG_WORD_LEN, // element count (each element is of size transfer_data_size)
                          true);
#endif
    uint8_t spi_byte;

        spi_hw_t *hw = spi_get_hw(spi0);

        printf("CR0:  %08x\n", hw->cr0);
        printf("CR1:  %08x\n", hw->cr1);
        // printf("DR:   %08x\n", hw->dr);
        printf("SR:   %08x\n", hw->sr);
        printf("CPSR: %08x\n", hw->cpsr);
        printf("IMSC: %08x\n", hw->imsc);
        printf("RIS:  %08x\n", hw->ris);
        printf("MIS:  %08x\n", hw->mis);
        printf("ICR:  %08x\n", hw->icr);
        printf("DMACR:%08x\n", hw->dmacr);
        printf("-----\n");
    while(true) {
        // Monitor registers during transfer
        


        if (!(gpio_get(PICO_DEFAULT_SPI_CSN_PIN))) {  // If CS is low
            // Print all relevant registers
            if (hw->sr & SPI_SSPSR_RNE_BITS) {
                printf("CR0:  %08x\n", hw->cr0);
                printf("CR1:  %08x\n", hw->cr1);
                // printf("DR:   %08x\n", hw->dr);
                printf("SR:   %08x\n", hw->sr);
                printf("CPSR: %08x\n", hw->cpsr);
                printf("IMSC: %08x\n", hw->imsc);
                printf("RIS:  %08x\n", hw->ris);
                printf("MIS:  %08x\n", hw->mis);
                printf("ICR:  %08x\n", hw->icr);
                printf("DMACR:%08x\n", hw->dmacr);
                uint32_t data = hw->dr;
                printf("Data: %02x\n", (uint8_t)data);
                data = hw->dr;
                printf("      %02x\n", (uint8_t)data);
                data = hw->dr;
                printf("      %02x\n", (uint8_t)data);
                data = hw->dr;
                printf("      %02x\n", (uint8_t)data);
            }
            printf("\n");
        }
        
        // sleep_ms(1);  // Small delay to keep output readable        
    }

#if 0
    while(true) {
        // if(examine_rx_buffer) {
        //     printbuf(spi_rx_buffer, 4);
        //     examine_rx_buffer = false;
        //     pico_set_led(true);
        //     // sleep_ms(10);
        //     // pico_set_led(false);
        // }
        printf("CS state: %d\n", gpio_get(PICO_DEFAULT_SPI_CSN_PIN));
        // printf("Reading byte...\n");
        // spi_read_blocking(spi0, 0, &spi_byte, 1);
        // printf("%02x\n", spi_byte);

        while(!(spi_get_hw(spi0)->sr & SPI_SSPSR_RNE_BITS)) {
            printf("SR: 0x%08X\n", spi_get_hw(spi0)->sr);
            sleep_ms(100);
        }
        
        // Read the byte
        spi_byte = (uint8_t)spi_get_hw(spi0)->dr;
        printf("Got byte: 0x%02X\n", spi_byte);

        printf("SPI SR: 0x%08X\n", spi_get_hw(spi0)->sr);
        printf("SPI DR: 0x%08X\n", spi_get_hw(spi0)->dr);

        // printbuf(spi_rx_buffer, 16);

        // printf("TICK %6d\n", iteration);
        // sleep_ms(100);
        // pico_set_led(false);
        sleep_ms(100);
        // iteration++;
    }
#endif
}
