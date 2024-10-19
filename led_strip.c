/**
 * Copyright (c) 2020 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <stdio.h>
#include <stdlib.h>
#include "math.h"
#include "pico/stdlib.h"
#include "hardware/spi.h"
#include "hardware/interp.h"
#include "hardware/adc.h"
#include "pico/rand.h"



#define FRAME_RATE_MS 1000/24


typedef struct {
    unsigned int unused : 3;
    unsigned int brightness : 5;
    unsigned int blue : 8;
    unsigned int green : 8;
    unsigned int red : 8;
} APA102_LED;

#define LED_STRIP_LEN 60
APA102_LED strip[LED_STRIP_LEN];


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


// Perform initialisation
int pico_led_init(void) {
    gpio_init(PICO_DEFAULT_LED_PIN);
    gpio_set_dir(PICO_DEFAULT_LED_PIN, GPIO_OUT);
    return PICO_OK;
}

// Turn the led on or off
void pico_set_led(bool led_on) {
    // Just set the GPIO on or off
    gpio_put(PICO_DEFAULT_LED_PIN, led_on);
}


static inline uint32_t apa102_command(uint8_t brightness, uint8_t red, uint8_t green, uint8_t blue) {
    return (0x7 << 29) || 
           ((uint32_t)(brightness & 0x1f) << 24) || 
           ((uint32_t)(blue & 0xff) << 16) || 
           ((uint32_t)(green & 0xff) << 8) || 
           (red & 0xff);
}

void apa102_strip_update(APA102_LED *strip, uint16_t led_count) {
    uint32_t start_frame = 0, end_frame = 0xffffffff;
    spi_write_blocking(spi_default, (uint8_t *)&start_frame, sizeof(uint32_t));
    spi_write_blocking(spi_default, (uint8_t *)strip, led_count * 4);
    spi_write_blocking(spi_default, (uint8_t *)&end_frame, sizeof(uint32_t));
}


void apa102_strip_init(APA102_LED *strip, uint16_t led_count) {
    // Initialize strip
    for(int i = 0; i < LED_STRIP_LEN; i++) {
        strip[i].unused = 0x7;
        strip[i].brightness = 0;
        strip[i].red = 0;
        strip[i].green = 0;
        strip[i].blue = 0;
    }
}

typedef struct {
    uint16_t hue;
    uint16_t saturation;
    uint16_t lightness;
} HSL;

typedef struct 
{
    uint8_t red;
    uint8_t green;
    uint8_t blue;
} RGB;

uint8_t clamp(int value) {
    if (value < 0) return 0;
    if (value > 255) return 255;
    return (uint8_t)value;
}

/**
 * Convert HSL to RGB.
 * 
 * Hue is in the range of 0 to 359
 * Saturation is in the range of 0 to 999
 * Lightness is in the range of 0 to 999
 * 
 * Red, green, and blue outputs are unsigned 8-bit integers
 */
void hsl_to_rgb(HSL *hsl, RGB *rgb) {
    // printf("CONVERTING %d, %d, %d to RGB\n", hsl->hue, hsl->saturation, hsl->lightness);

    // Normalize saturation and lightness to the range 0.0 - 1.0
    float s_f = hsl->saturation / 1000.0;
    printf("%1.7f,", s_f);
    float l_f = hsl->lightness / 1000.0;
    printf("%1.7f,", l_f);

    // Calculate chroma
    float c = (1.0f - fabs(2.0f * l_f - 1.0f)) * s_f;
    printf("%1.7f,", c);

    float x = c * (1.0f - fabs(fmod(hsl->hue / 60.0, 2) - 1.0f));
    printf("%1.7f,", x);
    float m = l_f - c / 2.0;
    printf("%1.7f,", m);

    float r1 = 0, g1 = 0, b1 = 0;

    // Determine the RGB1 values based on the hue segment
    if (hsl->hue < 60) {
        r1 = c; g1 = x; b1 = 0;
    } else if (hsl->hue < 120) {
        r1 = x; g1 = c; b1 = 0;
    } else if (hsl->hue < 180) {
        r1 = 0; g1 = c; b1 = x;
    } else if (hsl->hue < 240) {
        r1 = 0; g1 = x; b1 = c;
    } else if (hsl->hue < 300) {
        r1 = x; g1 = 0; b1 = c;
    } else {
        r1 = c; g1 = 0; b1 = x;
    }

    printf("%1.7f,%1.7f,%1.7f,",r1,g1,b1);

    // Convert to 0-255 range and store the values in the RGB variables
    rgb->red = clamp((r1 + m) * 255);
    rgb->green = clamp((g1 + m) * 255);
    rgb->blue = clamp((b1 + m) * 255);

    // printf("RGB=%d/%d/%d\n",rgb->red,rgb->green,rgb->blue);
}

uint32_t iteration;
uint16_t next_hue = 0, last_hue = 0;
float fitered_h = 180.0, filtered_l = 500.0, filtered_s = 500.0;
float filtered_r = 1.0, filtered_g = 1.0, filtered_b = 1.0;

#define FILTER_ALPHA 0.05
float filter_float(float previous, float new) {
    return ((FILTER_ALPHA * new) + ((1.0 - FILTER_ALPHA) * previous));
}

bool led_strip_frame_callback(__unused struct repeating_timer *t) {

    uint16_t adc_hue, adc_lightness, adc_saturation;

    adc_select_input(0);
    adc_hue = adc_read();
    adc_select_input(1);
    adc_lightness = adc_read();
    adc_select_input(2);
    adc_saturation = adc_read();

    RGB rgb;
    HSL hsl;

    // fitered_h = filter_float(fitered_h, adc_hue / 4096.0 * 360.0);
    // filtered_l = filter_float(filtered_l, adc_lightness / 4096.0 * 1000.0);
    // filtered_s = filter_float(filtered_s, adc_saturation / 4096.0 * 1000.0);

    // hsl.hue = (uint16_t)fitered_h;
    // hsl.lightness = (uint16_t)filtered_l;
    // hsl.saturation = (uint16_t)filtered_s;

    // printf("%04d,%04d,%04d,%03d,%03d,%03d,",
    //        adc_hue, adc_saturation, adc_lightness,
    //        hsl.hue, hsl.saturation, hsl.lightness);


    filtered_r = filter_float(filtered_r, adc_hue / 4096.0 * 256.0);
    filtered_g = filter_float(filtered_g, adc_lightness / 4096.0 * 256.0);
    filtered_b = filter_float(filtered_b, adc_saturation / 4096.0 * 256.0);
    rgb.red = (uint8_t)filtered_r;
    rgb.green = (uint8_t)filtered_g;
    rgb.blue = (uint8_t)filtered_b;

    // hsl_to_rgb(&hsl, &rgb);

    printf("%03d,%03d,%03d\n",rgb.red, rgb.green, rgb.blue);

    for(int i = 0; i < LED_STRIP_LEN; i++) {
        strip[i].green = rgb.green;
        strip[i].red = rgb.red;
        strip[i].blue = rgb.blue;
    }

#if 0
    int16_t animation_step = (iteration % 60) - 5;
    for(int i = 0; i < LED_STRIP_LEN; i++) {
        uint16_t distance_from_step = abs(i - animation_step);
        int16_t lightness = 500 - (distance_from_step * 100);
        if(lightness < 0) {
            hsl.lightness = 50;
            hsl.saturation = 50;
            hsl.hue = last_hue;
        }
        else {
            hsl.lightness = lightness;
            hsl.saturation = lightness >> 2;
            hsl.hue = next_hue;
        }

        hsl_to_rgb(&hsl, &rgb);
        
        strip[i].green = rgb.green;
        strip[i].red = rgb.red;
        strip[i].blue = rgb.blue;
    }

    if((iteration % 240) == 0) {
        last_hue = next_hue;
        next_hue = get_rand_32() % 360;
        printf("NEW TARGET HUE=%d\n", next_hue);
    }

    iteration++;
#endif

     // printf("iteration %d\n", iteration);
    pico_set_led(true);
    apa102_strip_update(strip, LED_STRIP_LEN);
    pico_set_led(false);

    return true;
}


int main() {
    printf("APA102 LED strip control\n");
    pico_led_init();
    pico_set_led(true);
    stdio_init_all();

    adc_init();
    adc_gpio_init(26);

    spi_init(spi_default, 1000 * 1000);
    gpio_set_function(PICO_DEFAULT_SPI_RX_PIN, GPIO_FUNC_SPI);
    gpio_set_function(PICO_DEFAULT_SPI_SCK_PIN, GPIO_FUNC_SPI);
    gpio_set_function(PICO_DEFAULT_SPI_TX_PIN, GPIO_FUNC_SPI);
    gpio_set_function(PICO_DEFAULT_SPI_CSN_PIN, GPIO_FUNC_SPI);

    apa102_strip_init(strip, LED_STRIP_LEN);

    struct repeating_timer timer;
    add_repeating_timer_ms(FRAME_RATE_MS, led_strip_frame_callback, NULL, &timer);


    float g1 = 0.067694;
    float m = 0.147153;
    float g = clamp((g1 + m) * 255);

    while(true) {
        // printf("g1=%1.7f\tm=%1.7f\tg=%1.7f\n", g1, m, g);
        sleep_ms(1000);
    }
}
