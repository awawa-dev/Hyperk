// File: include/leds.h
#pragma once
#include <Arduino.h>
#include <vector>
#include "config.h"

namespace Leds {
    bool supportsDoubleBuffering();
    void tryWaitForRenderer();
    void applyLedConfig();
    bool restartRequired();
    int getLedsNumber();
    int segmentSupported();
    void checkDelayedRender();
    void renderLed(bool isNewFrame);
    void synchronizeLedsToVolatileStateBeforeDelayedRender();

    template<bool applyBrightness>
    void setLed(int index, uint8_t r, uint8_t g, uint8_t b);

    template<bool applyBrightness>
    void setLedW(int index, uint8_t r, uint8_t g, uint8_t b, uint8_t w);
};

