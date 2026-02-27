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
    unsigned long streamTimeout = 0;
};

namespace Volatile{
    extern const VolatileState& state;

    static constexpr unsigned long defaultStreamTimeout = 6500;

    void updateBrightness(uint8_t  brightness);
    void updatePowerOn(bool on);
    void updateStaticColor(uint8_t red, uint8_t green, uint8_t blue);
    void updateStreamTimeout(unsigned long timeout = defaultStreamTimeout);

    void checkStreamTimeout();

    bool clearUpdatedBrightnessState();
    bool clearUpdatedPowerOnState();
    bool clearUpdatedStaticColorState();
};
