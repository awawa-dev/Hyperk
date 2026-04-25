/* fastled_bridge.h
*
*  MIT License
*
*  Copyright (c) 2026 awawa-dev
*
*  Project homesite: https://github.com/awawa-dev/Hyperk
*
*  Permission is hereby granted, free of charge, to any person obtaining a copy
*  of this software and associated documentation files (the "Software"), to deal
*  in the Software without restriction, including without limitation the rights
*  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
*  copies of the Software, and to permit persons to whom the Software is
*  furnished to do so, subject to the following conditions:
*
*  The above copyright notice and this permission notice shall be included in all
*  copies or substantial portions of the Software.

*  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
*  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
*  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
*  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
*  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
*  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
*  SOFTWARE.
*/

#pragma once

#include <FastLED.h>
#include "led_bridge.h"

struct fastled_bridge : public led_bridge
{
    CRGB* leds = nullptr;
    uint16_t fastLedsNumber = 0;
    LedType fastLedsType = LedType::WS2812;

    int getLedsNumber() override
    {
        return fastLedsNumber;
    }

    void clearAll() override
    {
        FastLED.clear(true);
    }

    bool canRender() override
    {
        return true;
    }

    bool executeRenderLed(bool isNewFrame) override
    {
        FastLED.show();
        return true;
    }

    void releaseDriverResources() override
    {        
        if (leds != nullptr)
        {
            delete[] leds;
            leds = nullptr;
        }
    }

    void initializeLedDriver(LedType cfgLedType, uint16_t cfgLedNumLeds, uint8_t cfgLedDataPin, uint8_t cfgLedClockPin,
                            uint8_t calGain, uint8_t calRed, uint8_t calGreen, uint8_t calBlue) override
    {
        fastLedsNumber = cfgLedNumLeds;
        fastLedsType = cfgLedType;
        int virtualLedsNumber = fastLedsNumber;

        if (fastLedsType == LedType::SK6812)
        {
            setParamsAndPrepareCalibration(calGain, calRed, calGreen, calBlue);
            virtualLedsNumber = (fastLedsNumber * 4 + 2) / 3;
            leds = new CRGB[virtualLedsNumber];
        }
        else
        {
            leds = new CRGB[virtualLedsNumber];
        }

        if (fastLedsType == LedType::WS2812 || fastLedsType == LedType::SK6812)
        {
            switch (cfgLedDataPin) {
                #if !defined(CONFIG_IDF_TARGET_ESP32S3)
                case 0: FastLED.addLeds<WS2812, 0, RGB>(leds, virtualLedsNumber); break;
                #endif
                case 1: FastLED.addLeds<WS2812, 1, RGB>(leds, virtualLedsNumber); break;
                case 2: FastLED.addLeds<WS2812, 2, RGB>(leds, virtualLedsNumber); break;
                #if !defined(CONFIG_IDF_TARGET_ESP32S3)
                case 3: FastLED.addLeds<WS2812, 3, RGB>(leds, virtualLedsNumber); break;
                #endif
                case 4: FastLED.addLeds<WS2812, 4, RGB>(leds, virtualLedsNumber); break;
                case 5: FastLED.addLeds<WS2812, 5, RGB>(leds, virtualLedsNumber); break;
                case 6: FastLED.addLeds<WS2812, 6, RGB>(leds, virtualLedsNumber); break;
                case 7: FastLED.addLeds<WS2812, 7, RGB>(leds, virtualLedsNumber); break;
                #if !defined(CONFIG_IDF_TARGET_ESP32C2)
                case 8: FastLED.addLeds<WS2812, 8, RGB>(leds, virtualLedsNumber); break;
                #endif
                case 10: FastLED.addLeds<WS2812, 10, RGB>(leds, virtualLedsNumber); break;
                #if defined(CONFIG_IDF_TARGET_ESP32C6)
                case 15: FastLED.addLeds<WS2812, 15, RGB>(leds, virtualLedsNumber); break;
                case 18: FastLED.addLeds<WS2812, 18, RGB>(leds, virtualLedsNumber); break;
                case 19: FastLED.addLeds<WS2812, 19, RGB>(leds, virtualLedsNumber); break;
                case 20: FastLED.addLeds<WS2812, 20, RGB>(leds, virtualLedsNumber); break;
                case 21: FastLED.addLeds<WS2812, 21, RGB>(leds, virtualLedsNumber); break;
                case 22: FastLED.addLeds<WS2812, 22, RGB>(leds, virtualLedsNumber); break;
                #elif defined(CONFIG_IDF_TARGET_ESP32S3)
                case 16: FastLED.addLeds<WS2812, 16, RGB>(leds, virtualLedsNumber); break;
                case 17: FastLED.addLeds<WS2812, 17, RGB>(leds, virtualLedsNumber); break;
                case 18: FastLED.addLeds<WS2812, 18, RGB>(leds, virtualLedsNumber); break;
                case 48: FastLED.addLeds<WS2812, 48, RGB>(leds, virtualLedsNumber); break;
                #elif defined(CONFIG_IDF_TARGET_ESP32C3)
                case 20: FastLED.addLeds<WS2812, 20, RGB>(leds, virtualLedsNumber); break;
                case 21: FastLED.addLeds<WS2812, 21, RGB>(leds, virtualLedsNumber); break;
                #elif defined(CONFIG_IDF_TARGET_ESP32C5)
                case 11: FastLED.addLeds<WS2812, 11, RGB>(leds, virtualLedsNumber); break;
                case 27: FastLED.addLeds<WS2812, 27, RGB>(leds, virtualLedsNumber); break;
                #endif
                default:
                    FastLED.addLeds<WS2812, 2, RGB>(leds, virtualLedsNumber);
                    break;
            }
        }
        else
        { // SPI (APA102 / SK9822)
            switch (cfgLedDataPin) {
                #if defined(CONFIG_IDF_TARGET_ESP32S3)
                    case 5: FastLED.addLeds<APA102, 5, 4, BRG>(leds, virtualLedsNumber); break;
                #elif defined(CONFIG_IDF_TARGET_ESP32C2) || defined(CONFIG_IDF_TARGET_ESP32C3) || defined(CONFIG_IDF_TARGET_ESP32C5)
                    case 7:  FastLED.addLeds<APA102, 7, 6, BRG>(leds, virtualLedsNumber); break;
                #elif defined(CONFIG_IDF_TARGET_ESP32C6)
                    case 5:  FastLED.addLeds<APA102, 5, 4, BRG>(leds, virtualLedsNumber); break;
                #endif
                
                default:                        
                        Log::debug("!!! FATAL ERROR: Invalid LED Data Pin. Must use Hardware SPI pins. !!!");
                        FastLED.addLeds<APA102, 7, 6, BRG>(leds, virtualLedsNumber);
            }
        }

        FastLED.setBrightness(255);
    }

    inline void setLedRgb(int index, uint8_t r, uint8_t g, uint8_t b) override
    {
        if (fastLedsType == LedType::SK6812)
        {            
            if (index >= fastLedsNumber) return;
            const ColorRgbw calibrated = rgb2rgbw(r, g, b);
            uint16_t i = index * 4;
            auto raw = reinterpret_cast<uint8_t*>(leds);
            raw[i]     = calibrated.G;
            raw[i + 1] = calibrated.R;
            raw[i + 2] = calibrated.B;
            raw[i + 3] = calibrated.W;
        }
        else
        {
            if (index >= fastLedsNumber) return;
            leds[index] = CRGB(g, r, b);
        }
    }

    inline void setLedRgbw(int index, uint8_t r, uint8_t g, uint8_t b, uint8_t w) override
    {
        if (fastLedsType == LedType::SK6812)
        {
            if (index >= fastLedsNumber) return;
            uint16_t i = index * 4;
            auto raw = reinterpret_cast<uint8_t*>(leds);
            raw[i]     = g;
            raw[i + 1] = r;
            raw[i + 2] = b;
            raw[i + 3] = w;
        }
        else
        {
            if (index >= fastLedsNumber) return;
            leds[index] = CRGB(g, r, b);
        }
    }
};