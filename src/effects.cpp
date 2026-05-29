#include "effects.h"

namespace EffectUtils{
    void fast_hsv2rgb(uint8_t h, uint8_t s, uint8_t v, uint8_t *r, uint8_t *g, uint8_t *b) {
        if (s == 0) {
            *r = *g = *b = v;
            return;
        }

        uint8_t region = h / 43;
        uint8_t remainder = (h - region * 43) * 6;

        uint8_t p = (uint16_t)v * (255 - s) >> 8;
        uint8_t q = (uint16_t)v * (255 - ((uint16_t)s * remainder >> 8)) >> 8;
        uint8_t t = (uint16_t)v * (255 - ((uint16_t)s * (255 - remainder) >> 8)) >> 8;

        switch (region) {
            case 0:  *r = v; *g = t; *b = p; break;
            case 1:  *r = q; *g = v; *b = p; break;
            case 2:  *r = p; *g = v; *b = t; break;
            case 3:  *r = p; *g = q; *b = v; break;
            case 4:  *r = t; *g = p; *b = v; break;
            default: *r = v; *g = p; *b = q; break;
        }
    }

    void fast_rgb2hsv(uint8_t r, uint8_t g, uint8_t b, uint8_t *h, uint8_t *s, uint8_t *v) {
        uint8_t rgbMin = r < g ? (r < b ? r : b) : (g < b ? g : b);
        uint8_t rgbMax = r > g ? (r > b ? r : b) : (g > b ? g : b);

        *v = rgbMax;

        if (*v == 0) {
            *h = *s = 0;
            return;
        }

        uint8_t delta = rgbMax - rgbMin;

        if (delta == 0) {
            *h = 0;
            *s = 0;
            return;
        }

        *s = (uint16_t)255 * delta / rgbMax;

        int16_t hue;
        if (rgbMax == r)
            hue = 43 * (int16_t)(g - b) / delta;
        else if (rgbMax == g)
            hue = 85 + 43 * (int16_t)(b - r) / delta;
        else
            hue = 171 + 43 * (int16_t)(r - g) / delta;

        if (hue < 0) hue += 256;
        *h = (uint8_t)hue;
    }      
};
