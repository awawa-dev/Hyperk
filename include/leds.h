// File: include/leds.h
#pragma once
#include <Arduino.h>
#include <vector>
#include "config.h"

#if !(defined(USE_FASTLED) || defined(ARDUINO_ARCH_RP2040) || defined(ARDUINO_ARCH_RP2350))
    #define LEDS_NOT_REQUIRE_RESTART
#endif

namespace Leds {
    void applyLedConfig();
    int getLedsNumber();
    void checkDelayedRender();
    void renderLed(bool isNewFrame);
    void synchronizeLedsToVolatileStateBeforeDelayedRender();

    template<bool applyBrightness>
    void setLed(int index, uint8_t r, uint8_t g, uint8_t b);

    template<bool applyBrightness>
    void setLedW(int index, uint8_t r, uint8_t g, uint8_t b, uint8_t w);
};

