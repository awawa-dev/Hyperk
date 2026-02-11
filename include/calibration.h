// File: include/calibration.h
#pragma once

#ifdef USE_FASTLED
    struct RgbwColor {
        uint8_t R, G, B, W;
    };
#endif

void setParamsAndPrepareCalibration(uint8_t _gain, uint8_t _red, uint8_t _green, uint8_t _blue);
void deleteCalibration();
RgbwColor rgb2rgbw(uint8_t r, uint8_t g, uint8_t b);

