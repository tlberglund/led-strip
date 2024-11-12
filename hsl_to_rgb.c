
#include "hsl_to_rgb.h"

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