/* leds.cpp
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

#ifdef USE_FASTLED
    #include <FastLED.h>
    #include "config.h"

    namespace
    {
        CRGB* leds = nullptr;
        uint16_t fastLedsNumber = 0;
        LedType fastLedsType = LedType::WS2812;
    }    
#else
    #include <NeoPixelBus.h>

    #if defined(CONFIG_IDF_TARGET_ESP32C6)
        typedef NeoPixelBus<DotStarLbgrFeature, DotStarSpi10MhzMethod > DotStar;
        typedef NeoPixelBus<NeoGrbFeature, NeoEsp32BitBangWs2812Method > NeoPixel;
        typedef NeoPixelBus<NeoGrbwFeature, NeoEsp32BitBangWs2812Method > NeoPixelRgbw;
    #elif defined(CONFIG_IDF_TARGET_ESP32C3)
        typedef NeoPixelBus<DotStarLbgrFeature, DotStarSpi10MhzMethod > DotStar;
        typedef NeoPixelBus<NeoGrbFeature, NeoEsp32Rmt0Ws2812xMethod > NeoPixel;
        typedef NeoPixelBus<NeoGrbwFeature, NeoEsp32Rmt0Sk6812Method > NeoPixelRgbw;
    #elif defined(CONFIG_IDF_TARGET_ESP32S3)
        typedef NeoPixelBus<DotStarLbgrFeature, DotStarSpi10MhzMethod > DotStar;
        typedef NeoPixelBus<NeoGrbFeature, NeoEsp32LcdX8Ws2812Method > NeoPixel;
        typedef NeoPixelBus<NeoGrbwFeature, NeoEsp32LcdX8Sk6812Method > NeoPixelRgbw;    
    #elif defined(CONFIG_IDF_TARGET_ESP32S2)
        typedef NeoPixelBus<DotStarLbgrFeature, DotStarSpi10MhzMethod > DotStar;
        typedef NeoPixelBus<NeoGrbFeature, NeoEsp32I2s0Ws2812xMethod> NeoPixel;
        typedef NeoPixelBus<NeoGrbwFeature, NeoEsp32I2s0Sk6812Method> NeoPixelRgbw;
    #elif defined(ARDUINO_ARCH_ESP32)
        typedef NeoPixelBus<DotStarLbgrFeature, DotStarSpi10MhzMethod > DotStar;
        typedef NeoPixelBus<NeoGrbFeature, NeoEsp32I2s1800KbpsMethod> NeoPixel;
        typedef NeoPixelBus<NeoGrbwFeature, NeoEsp32I2s1800KbpsMethod> NeoPixelRgbw;
    #elif defined(ARDUINO_ARCH_ESP8266)
        typedef NeoPixelBus<DotStarLbgrFeature, DotStarSpiMethod> DotStar;
        typedef NeoPixelBus<NeoGrbFeature, NeoEsp8266Uart1Ws2812xMethod> NeoPixel;
        typedef NeoPixelBus<NeoGrbwFeature, NeoEsp8266Uart1Sk6812Method> NeoPixelRgbw;
    #else // Raspberry Pi Pico
        typedef NeoPixelBus<DotStarLbgrFeature, DotStarSpi10MhzMethod> DotStar;
        typedef NeoPixelBus<NeoGrbFeature, Rp2040x4Pio0Ws2812xMethod> NeoPixel;
        typedef NeoPixelBus<NeoGrbwFeature, Rp2040x4Pio0Ws2812xMethod> NeoPixelRgbw;
    #endif

    namespace
    {
        DotStar* dotstar = nullptr;
        NeoPixel* neopixel = nullptr;
        NeoPixelRgbw* neopixelRgbw = nullptr;
    }
#endif

#include "leds.h"
#include "config.h"
#include "storage.h"
#include "manager.h"
#include "calibration.h"

//////////////////////////////////////////////////////////////////////////////////////////////////

namespace{
    volatile bool delayedRender = false;
}

int getLedsNumber()
{
    #ifdef USE_FASTLED
        return fastLedsNumber;
    #else    
        if (dotstar != nullptr) 
            return dotstar->PixelCount();
        else if (neopixel != nullptr)
            return neopixel->PixelCount();
        else if (neopixelRgbw != nullptr)
            return neopixelRgbw->PixelCount();
        
        return 0;
    #endif
}

void clearAll()
{
    #ifdef USE_FASTLED
        FastLED.clear(true);
    #else
        if (dotstar == nullptr && neopixel == nullptr && neopixelRgbw == nullptr) 
            return;

        if (dotstar != nullptr) 
            {dotstar->ClearTo(RgbwColor(0, 0, 0, 31)); dotstar->Show();}
        else if (neopixel != nullptr)
            {neopixel->ClearTo(RgbColor(0, 0, 0)); neopixel->Show();}
        else if (neopixelRgbw != nullptr)
            {neopixelRgbw->ClearTo(RgbwColor(0, 0, 0, 0)); neopixelRgbw->Show();}
    #endif
}

void initLEDs(LedType cfgLedType, uint16_t cfgLedNumLeds, uint8_t cfgLedDataPin, uint8_t cfgLedClockPin,
                uint8_t calGain, uint8_t calRed, uint8_t calGreen, uint8_t calBlue) {
    clearAll();

    #ifdef USE_FASTLED
        if (leds != nullptr)
        {
            delete[] leds;
            leds = nullptr;
        }
    #else
        if (dotstar != nullptr || neopixel != nullptr || neopixelRgbw != nullptr) 
        { 
                delay(50);
                delete dotstar; dotstar = nullptr;
                delete neopixel; neopixel = nullptr; 
                delete neopixelRgbw; neopixelRgbw = nullptr;
                delay(100);
        }    
    #endif

    if (cfgLedType != LedType::SK6812)
    {
        deleteCalibration();
    }

    delayedRender = false;

    // LED controller setup
    #ifdef USE_FASTLED
        fastLedsNumber = cfgLedNumLeds;
        fastLedsType = cfgLedType;
        int virtualLedsNumber = fastLedsNumber;

        if (fastLedsType == LedType::SK6812)
        {
            virtualLedsNumber = (fastLedsNumber * 4 + 2) / 3;
            const size_t bytesToAllocate = virtualLedsNumber * 3;
            leds = reinterpret_cast<CRGB*>(new uint8_t[bytesToAllocate]);
            memset(leds, 0, bytesToAllocate);
        }
        else
        {
            leds = new CRGB[virtualLedsNumber];
        }

        if (fastLedsType == LedType::WS2812 || fastLedsType == LedType::SK6812)
        {
            switch (cfgLedDataPin) {
                case 0: FastLED.addLeds<WS2812, 0, GRB>(leds, virtualLedsNumber); break;
                case 1: FastLED.addLeds<WS2812, 1, GRB>(leds, virtualLedsNumber); break;
                case 2: FastLED.addLeds<WS2812, 2, GRB>(leds, virtualLedsNumber); break;
                case 3: FastLED.addLeds<WS2812, 3, GRB>(leds, virtualLedsNumber); break;
                case 4: FastLED.addLeds<WS2812, 4, GRB>(leds, virtualLedsNumber); break;
                case 5: FastLED.addLeds<WS2812, 5, GRB>(leds, virtualLedsNumber); break;
                case 6: FastLED.addLeds<WS2812, 6, GRB>(leds, virtualLedsNumber); break;
                case 7: FastLED.addLeds<WS2812, 7, GRB>(leds, virtualLedsNumber); break;
                case 15: FastLED.addLeds<WS2812, 15, GRB>(leds, virtualLedsNumber); break;
                case 16: FastLED.addLeds<WS2812, 16, GRB>(leds, virtualLedsNumber); break;
                case 17: FastLED.addLeds<WS2812, 17, GRB>(leds, virtualLedsNumber); break;
                case 18: FastLED.addLeds<WS2812, 18, GRB>(leds, virtualLedsNumber); break;
                case 19: FastLED.addLeds<WS2812, 19, GRB>(leds, virtualLedsNumber); break;
                default:
                    FastLED.addLeds<WS2812, 2, GRB>(leds, virtualLedsNumber);
                    break;
            }
        }
        else
        { // SPI (APA102 / SK9822)            
            switch (cfgLedDataPin) {
                case 6:  FastLED.addLeds<APA102, 6, 7, BGR>(leds, virtualLedsNumber); break;
                case 18: FastLED.addLeds<APA102, 18, 19, BGR>(leds, virtualLedsNumber); break;
                case 0:  FastLED.addLeds<APA102, 0, 1, BGR>(leds, virtualLedsNumber); break;
                case 4:  FastLED.addLeds<APA102, 4, 5, BGR>(leds, virtualLedsNumber); break;
                default:
                    FastLED.addLeds<APA102, 6, 7, BGR>(leds, virtualLedsNumber); 
                    break;
            }
        }
    #else
        if (cfgLedType == LedType::WS2812 || cfgLedType == LedType::SK6812)
        { // clockless
            switch (cfgLedType)
            {
                case LedType::SK6812:
                    setParamsAndPrepareCalibration(calGain, calRed, calGreen, calBlue);
                    neopixelRgbw = new NeoPixelRgbw(cfgLedNumLeds, cfgLedDataPin);
                    neopixelRgbw->Begin();
                    break;
                default:
                    neopixel = new NeoPixel(cfgLedNumLeds, cfgLedDataPin);
                    neopixel->Begin();
            }
        }
        else
        { // SPI (APA102 / SK9822)
            #if defined(ARDUINO_ARCH_ESP32)
                dotstar = new DotStar(cfgLedNumLeds, cfgLedDataPin, cfgLedClockPin); //23, 18
            #else 
                dotstar = new DotStar(cfgLedNumLeds, cfgLedDataPin, cfgLedClockPin); //19, 18
            #endif
            dotstar->Begin();
        }
    #endif

    clearAll();
}

void applyLedConfig()
{
    initLEDs(cfg.led.type, cfg.led.numLeds, cfg.led.dataPin, cfg.led.clockPin, cfg.led.calibration.gain, cfg.led.calibration.red, cfg.led.calibration.green, cfg.led.calibration.blue);
}

void setLed(int index, uint8_t r, uint8_t g, uint8_t b)
{
    #ifdef USE_FASTLED
        if (fastLedsType == LedType::SK6812)
        {            
            if (index >= fastLedsNumber) return;
            const RgbwColor calibrated = rgb2rgbw(r, g, b);
            uint16_t i = index * 4;
            reinterpret_cast<uint8_t*>(leds)[i]     = calibrated.G;
            reinterpret_cast<uint8_t*>(leds)[i + 1] = calibrated.R;
            reinterpret_cast<uint8_t*>(leds)[i + 2] = calibrated.B;
            reinterpret_cast<uint8_t*>(leds)[i + 3] = calibrated.W;
        }
        else
        {
            if (index >= fastLedsNumber) return;
            leds[index] = CRGB(r, g, b);
        }
    #else
        if (dotstar != nullptr) 
            dotstar->SetPixelColor(index, RgbwColor(r, g, b, 31));
        else if (neopixel != nullptr)
            neopixel->SetPixelColor(index, RgbColor(r, g, b));
        else if (neopixelRgbw != nullptr)
            neopixelRgbw->SetPixelColor(index, rgb2rgbw(r, g, b));
    #endif
}

void setLed(int index, uint8_t r, uint8_t g, uint8_t b, uint8_t w)
{
    #ifdef USE_FASTLED
        if (fastLedsType == LedType::SK6812)
        {
            if (index >= fastLedsNumber) return;
            uint16_t i = index * 4;
            reinterpret_cast<uint8_t*>(leds)[i]     = g;
            reinterpret_cast<uint8_t*>(leds)[i + 1] = r;
            reinterpret_cast<uint8_t*>(leds)[i + 2] = b;
            reinterpret_cast<uint8_t*>(leds)[i + 3] = w;
        }
        else
        {
            if (index >= fastLedsNumber) return;
            leds[index] = CRGB(r, g, b);
        }
    #else
        if (dotstar != nullptr) 
            dotstar->SetPixelColor(index, RgbwColor(r, g, b, min(w, (uint8_t)31)));
        else if (neopixel != nullptr)
            neopixel->SetPixelColor(index, RgbColor(r, g, b));
        else if (neopixelRgbw != nullptr)
            neopixelRgbw->SetPixelColor(index, RgbwColor(r, g, b, w));
    #endif
}

void checkDeleyedRender()
{
    if (delayedRender)
    {
        renderLed(false);
    }
}

void queueRender(bool isNewFrame)
{
    if (isNewFrame && delayedRender)
    {
        stats.skippedFrames = stats.skippedFrames + 1;
    }
    delayedRender = true;
}

void renderLed(bool isNewFrame)
{
    #ifdef USE_FASTLED
        FastLED.show();
    #else
        if (dotstar != nullptr) 
        {
            if (!dotstar->CanShow())
            {
                queueRender(isNewFrame);
                return;
            }
            dotstar->Show();
        }
        else if (neopixel != nullptr)
        {
            if (!neopixel->CanShow())
            {
                queueRender(isNewFrame);
                return;
            }
            neopixel->Show();
        }
        else if (neopixelRgbw != nullptr)
        {
            if (!neopixelRgbw->CanShow())
            {
                queueRender(isNewFrame);
                return;
            }
            neopixelRgbw->Show();
        }
        else
            return;
    #endif

    delayedRender = false;
    stats.renderedFrames = stats.renderedFrames + 1;
}
