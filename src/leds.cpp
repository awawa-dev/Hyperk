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

    namespace Leds
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

    namespace Leds
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
#include "volatile_state.h"

//////////////////////////////////////////////////////////////////////////////////////////////////

namespace Leds{
    volatile bool delayedRender = false;

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

    bool canRender()
    {
        #ifndef USE_FASTLED
            if (dotstar != nullptr) 
            {
                return dotstar->CanShow();
            }
            else if (neopixel != nullptr)
            {
                return neopixel->CanShow();
            }
            else if (neopixelRgbw != nullptr)
            {
                return neopixelRgbw->CanShow();
            }
        #endif
        return true;
    }

    void synchronizeLedsToVolatileStateBeforeDeleyedRender()
    {
        if (delayedRender || !canRender())
            return;

        bool updated = false;
        if (Volatile::clearUpdatedBrightnessState())
        {
            updated = true;
            Log::debug("Updating brightness to: ", Volatile::state.brightness);
            #ifdef USE_FASTLED
                FastLED.setBrightness(Volatile::state.brightness);
            #endif
        }

        if (Volatile::clearUpdatedPowerOnState())
        {
            updated = true;
            Log::debug("Updating power on to: ", Volatile::state.on);
        }

        if (Volatile::clearUpdatedStaticColorState())
        {
            updated = true;
            Log::debug("Updating static color to: {", Volatile::state.staticColor.red, ", ", Volatile::state.staticColor.green, ", ", Volatile::state.staticColor.blue, "}");
        }

        if (updated)
        {
            auto r = (Volatile::state.on) ? Volatile::state.staticColor.red : 0;
            auto g = (Volatile::state.on) ? Volatile::state.staticColor.green : 0;
            auto b = (Volatile::state.on) ? Volatile::state.staticColor.blue : 0;

            #ifdef USE_FASTLED
                fill_solid(leds, getLedsNumber(), CRGB(r, g, b));
                FastLED.show();
            #else
                if (dotstar == nullptr && neopixel == nullptr && neopixelRgbw == nullptr) 
                    return;

                if (dotstar != nullptr) {
                    RgbwColor col(r, g, b, 31);
                    col = col.Dim(Volatile::state.brightness);
                    dotstar->ClearTo(col);
                    dotstar->Show();
                }
                else if (neopixel != nullptr) {
                    RgbColor col(r, g, b);
                    col = col.Dim(Volatile::state.brightness);
                    neopixel->ClearTo(col);
                    neopixel->Show();
                }
                else if (neopixelRgbw != nullptr) {
                    RgbwColor col(r, g, b, 0);
                    col = col.Dim(Volatile::state.brightness);
                    neopixelRgbw->ClearTo(col);
                    neopixelRgbw->Show();
                }
            #endif
        }
    }

    void initLEDs(LedType cfgLedType, uint16_t cfgLedNumLeds, uint8_t cfgLedDataPin, uint8_t cfgLedClockPin,
                    uint8_t calGain, uint8_t calRed, uint8_t calGreen, uint8_t calBlue) {
        clearAll();

        #ifdef USE_FASTLED
            if (FastLED.count())
            {
                return;
            }

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
                    #if !defined(CONFIG_IDF_TARGET_ESP32S3)
                    case 0: FastLED.addLeds<WS2812, 0, GRB>(leds, virtualLedsNumber); break;
                    #endif
                    case 1: FastLED.addLeds<WS2812, 1, GRB>(leds, virtualLedsNumber); break;
                    case 2: FastLED.addLeds<WS2812, 2, GRB>(leds, virtualLedsNumber); break;
                    #if !defined(CONFIG_IDF_TARGET_ESP32S3)
                    case 3: FastLED.addLeds<WS2812, 3, GRB>(leds, virtualLedsNumber); break;
                    #endif
                    case 4: FastLED.addLeds<WS2812, 4, GRB>(leds, virtualLedsNumber); break;
                    case 5: FastLED.addLeds<WS2812, 5, GRB>(leds, virtualLedsNumber); break;
                    case 6: FastLED.addLeds<WS2812, 6, GRB>(leds, virtualLedsNumber); break;
                    case 7: FastLED.addLeds<WS2812, 7, GRB>(leds, virtualLedsNumber); break;
                    #if !defined(CONFIG_IDF_TARGET_ESP32C2)
                    case 8: FastLED.addLeds<WS2812, 8, GRB>(leds, virtualLedsNumber); break;
                    #endif
                    case 10: FastLED.addLeds<WS2812, 10, GRB>(leds, virtualLedsNumber); break;
                    #if defined(CONFIG_IDF_TARGET_ESP32C6)
                    case 15: FastLED.addLeds<WS2812, 15, GRB>(leds, virtualLedsNumber); break;
                    case 18: FastLED.addLeds<WS2812, 18, GRB>(leds, virtualLedsNumber); break;
                    case 19: FastLED.addLeds<WS2812, 19, GRB>(leds, virtualLedsNumber); break;
                    case 20: FastLED.addLeds<WS2812, 20, GRB>(leds, virtualLedsNumber); break;
                    case 21: FastLED.addLeds<WS2812, 21, GRB>(leds, virtualLedsNumber); break;
                    case 22: FastLED.addLeds<WS2812, 22, GRB>(leds, virtualLedsNumber); break;
                    #elif defined(CONFIG_IDF_TARGET_ESP32S3)
                    case 16: FastLED.addLeds<WS2812, 16, GRB>(leds, virtualLedsNumber); break;
                    case 17: FastLED.addLeds<WS2812, 17, GRB>(leds, virtualLedsNumber); break;
                    case 18: FastLED.addLeds<WS2812, 18, GRB>(leds, virtualLedsNumber); break;
                    case 48: FastLED.addLeds<WS2812, 48, GRB>(leds, virtualLedsNumber); break;
                    #elif defined(CONFIG_IDF_TARGET_ESP32C3)
                    case 20: FastLED.addLeds<WS2812, 20, GRB>(leds, virtualLedsNumber); break;
                    case 21: FastLED.addLeds<WS2812, 21, GRB>(leds, virtualLedsNumber); break;
                    #elif defined(CONFIG_IDF_TARGET_ESP32C5)
                    case 11: FastLED.addLeds<WS2812, 11, GRB>(leds, virtualLedsNumber); break;
                    case 27: FastLED.addLeds<WS2812, 27, GRB>(leds, virtualLedsNumber); break;
                    #endif
                    default:
                        FastLED.addLeds<WS2812, 2, GRB>(leds, virtualLedsNumber);
                        break;
                }
            }
            else
            { // SPI (APA102 / SK9822)
                switch (cfgLedDataPin) {
                    #if defined(CONFIG_IDF_TARGET_ESP32S3)
                        case 11: FastLED.addLeds<APA102, 11, 12, BGR>(leds, virtualLedsNumber); break;
                    #elif defined(CONFIG_IDF_TARGET_ESP32C3)
                        case 7:  FastLED.addLeds<APA102, 7, 6, BGR>(leds, virtualLedsNumber); break;
                    #elif defined(CONFIG_IDF_TARGET_ESP32C6)
                        case 6:  FastLED.addLeds<APA102, 6, 5, BGR>(leds, virtualLedsNumber); break;
                    #endif
                    
                    default:                        
                            Log::debug("!!! FATAL ERROR: Invalid LED Data Pin. Must use Hardware SPI pins. !!!");
                            FastLED.addLeds<APA102, 7, 6, BGR>(leds, virtualLedsNumber);
                }
            }

            FastLED.setBrightness(Volatile::state.brightness);
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
        const AppConfig& cfg = Config::cfg;
        initLEDs(cfg.led.type, cfg.led.numLeds, cfg.led.dataPin, cfg.led.clockPin, cfg.led.calibration.gain, cfg.led.calibration.red, cfg.led.calibration.green, cfg.led.calibration.blue);
        Volatile::updateBrightness(cfg.led.brightness);
        Volatile::updatePowerOn(cfg.led.r || cfg.led.g || cfg.led.b);
        Volatile::updateStaticColor(cfg.led.r, cfg.led.g, cfg.led.b);
    }

    template<bool applyBrightness>
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
            {
                RgbwColor col(r, g, b, 31);
                if constexpr (applyBrightness) col = col.Dim(Volatile::state.brightness);
                dotstar->SetPixelColor(index, col);
            }
            else if (neopixel != nullptr)
            {
                RgbColor col(r, g, b);
                if constexpr (applyBrightness) col = col.Dim(Volatile::state.brightness);
                neopixel->SetPixelColor(index, col);
            }
            else if (neopixelRgbw != nullptr)
            {
                RgbwColor col = rgb2rgbw(r, g, b);
                if constexpr (applyBrightness) col = col.Dim(Volatile::state.brightness);
                neopixelRgbw->SetPixelColor(index, col);
            }
        #endif
    }

    template<bool applyBrightness>
    void setLedW(int index, uint8_t r, uint8_t g, uint8_t b, uint8_t w)
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
            {
                RgbwColor col(r, g, b, min(w, (uint8_t)31));
                if constexpr (applyBrightness) col = col.Dim(Volatile::state.brightness);
                dotstar->SetPixelColor(index, col);
            }
            else if (neopixel != nullptr)
            {
                RgbColor col(r, g, b);
                if constexpr (applyBrightness) col = col.Dim(Volatile::state.brightness);
                neopixel->SetPixelColor(index, col);
            }
            else if (neopixelRgbw != nullptr)
            {
                RgbwColor col(r, g, b, w);
                if constexpr (applyBrightness) col = col.Dim(Volatile::state.brightness);
                neopixelRgbw->SetPixelColor(index, col);
            }
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

    template void setLed<false>(int, uint8_t, uint8_t, uint8_t);
    template void setLedW<false>(int, uint8_t, uint8_t, uint8_t, uint8_t);

    #ifndef USE_FASTLED
        template void setLed<true>(int, uint8_t, uint8_t, uint8_t);
        template void setLedW<true>(int, uint8_t, uint8_t, uint8_t, uint8_t);
    #endif
}