/**
 * Copyright (c) 2020 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "pico/stdlib.h"
#include "hardware/adc.h"
#include "pico/rand.h"
#include "apa102.h"



#define FRAME_RATE_MS 1000/24


#define LED_STRIP_LEN 60
APA102_LED *strip;


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
    // printf("%1.7f,", s_f);
    float l_f = hsl->lightness / 1000.0;
    // printf("%1.7f,", l_f);

    // Calculate chroma
    float c = (1.0f - fabs(2.0f * l_f - 1.0f)) * s_f;
    // printf("%1.7f,", c);

    float x = c * (1.0f - fabs(fmod(hsl->hue / 60.0, 2) - 1.0f));
    // printf("%1.7f,", x);
    float m = l_f - c / 2.0;
    // printf("%1.7f,", m);

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

    // printf("%1.7f,%1.7f,%1.7f,",r1,g1,b1);

    // Convert to 0-255 range and store the values in the RGB variables
    rgb->red = clamp((r1 + m) * 255);
    rgb->green = clamp((g1 + m) * 255);
    rgb->blue = clamp((b1 + m) * 255);

    // printf("RGB=%d/%d/%d\n",rgb->red,rgb->green,rgb->blue);
}

uint32_t iteration;
float filtered_h = 180.0, filtered_l = 500.0, filtered_s = 500.0;

#define FILTER_ALPHA 0.1
float filter_float(float previous, float new) {
    return ((FILTER_ALPHA * new) + ((1.0 - FILTER_ALPHA) * previous));
}

bool led_strip_frame_callback(__unused struct repeating_timer *t) {
    RGB rgb;
    HSL hsl;

/*
    uint16_t adc_hue, adc_lightness, adc_saturation;
    adc_select_input(0);
    adc_hue = adc_read();
    adc_select_input(1);
    adc_lightness = adc_read();
    adc_select_input(2);
    adc_saturation = adc_read();


    filtered_h = filter_float(filtered_h, adc_hue / 4096.0 * 360.0);
    filtered_l = filter_float(filtered_l, adc_lightness / 4096.0 * 1000.0);
    filtered_s = filter_float(filtered_s, adc_saturation / 4096.0 * 1000.0);
    printf("%03.1f,%04.1f,%04.1f\n", filtered_h, filtered_s, filtered_l);

    hsl.hue = (uint16_t)fitered_h;
    hsl.lightness = (uint16_t)filtered_l;
    hsl.saturation = (uint16_t)filtered_s;

    printf("%04d,%04d,%04d,%03d,%03d,%03d,",
           adc_hue, adc_saturation, adc_lightness,
           hsl.hue, hsl.saturation, hsl.lightness);

    hsl_to_rgb(&hsl, &rgb);


    printf("%03d,%03d,%03d\n",rgb.red, rgb.green, rgb.blue);
    for(int i = 0; i < LED_STRIP_LEN; i++) {
        strip[i].green = rgb.green;
        strip[i].red = rgb.red;
        strip[i].blue = rgb.blue;
    }
*/

    // adc_select_input(0);
    // uint16_t adc_brightness = adc_read();
    // int8_t brightness = ((float)adc_brightness / (4096.0 / 16.0));
    // int16_t animation_step = (iteration % 80) - 15;

    adc_select_input(0);
    // uint16_t adc_hue = adc_read();
    // uint16_t adc_lightness = adc_read();
    // filtered_h = filter_float(filtered_h, adc_hue / 4096.0 * 360.0);
    // filtered_l = filter_float(filtered_l, adc_lightness / 4096.0 * 1000.0);
    // filtered_s = filter_float(filtered_s, adc_saturation / 4096.0 * 1000.0);

    uint16_t adc_color = adc_read();
    uint16_t color = adc_color >> 4;
    printf("%04u\t%04u\n", adc_color, color);


    // filtered_l = 500.0;
    // filtered_s = 850.0;
    // filtered_h = 180.0;
    // printf("%u\t%3.1f\n",adc_hue, filtered_h);

    // hsl.hue = (uint16_t)filtered_h;
    // hsl.lightness = (uint16_t)filtered_l;
    // hsl.saturation = (uint16_t)filtered_s;
    // hsl_to_rgb(&hsl, &rgb);
    rgb.red = 0x80;
    rgb.green = 0x55;
    rgb.blue = 0x01;

    for(int i = 0; i < LED_STRIP_LEN; i++) {
        /*
        uint16_t distance_from_step = abs(i - animation_step);
        int16_t lightness = filtered_l - (distance_from_step * 50);
        if(lightness < 0) {
            hsl.lightness = 0;
            hsl.saturation = filtered_s;
            hsl.hue = filtered_h;
        }
        else {
            hsl.lightness = lightness;
            hsl.saturation = filtered_s;
            hsl.hue = filtered_h;
        }
        */
        apa102_set_led(i, rgb.red, rgb.green, rgb.blue, 15);
    }
    iteration++;

    if(iteration & 1)
        pico_set_led(true);
    else
        pico_set_led(false);

    apa102_strip_update();

    return true;
}


int main() {
    printf("APA102 LED strip control\n");
    stdio_init_all();
    pico_led_init();
    pico_set_led(false);

    adc_init();
    adc_gpio_init(26);

    strip = apa102_init(LED_STRIP_LEN);
    apa102_log_strip();

    struct repeating_timer timer;
    add_repeating_timer_ms(FRAME_RATE_MS, led_strip_frame_callback, NULL, &timer);

    while(true) {
        // pico_set_led(true);
        // sleep_ms(1000);
        // pico_set_led(false);
        sleep_ms(1000);
    }
}
