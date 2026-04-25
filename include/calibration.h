// File: include/calibration.h
#pragma once

struct ColorRgbw {
    uint8_t R, G, B, W;
};

void setParamsAndPrepareCalibration(uint8_t _gain, uint8_t _red, uint8_t _green, uint8_t _blue);
void deleteCalibration();
ColorRgbw rgb2rgbw(uint8_t r, uint8_t g, uint8_t b);

