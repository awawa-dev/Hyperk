// File: include/config.h
#pragma once
#include <Arduino.h>

struct VolatileState {
    bool     on = false;
    bool     live = false;
    struct StaticColor
    {
        uint8_t  red = 0, green = 0, blue = 0;
    } staticColor;
    uint8_t  effect     = 0;
    uint8_t  brightness = 255;
};

namespace Volatile{
    extern const VolatileState& state;

    void updateBrightness(uint8_t  brightness);
    void updatePowerOn(bool on);
    void updateStaticColor(uint8_t red, uint8_t green, uint8_t blue);

    bool clearUpdatedBrightnessState();
    bool clearUpdatedPowerOnState();
    bool clearUpdatedStaticColorState();
};
