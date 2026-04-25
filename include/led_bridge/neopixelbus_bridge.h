/* neopixelbus_bridge.h
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
#include <NeoPixelBus.h>
#include "led_bridge.h"

#if defined(CONFIG_IDF_TARGET_ESP32C6)
    typedef NeoPixelBus<DotStarBgrFeature, DotStarSpi10MhzMethod > DotStar;
    typedef NeoPixelBus<NeoGrbFeature, NeoEsp32BitBangWs2812Method > NeoPixel;
    typedef NeoPixelBus<NeoGrbwFeature, NeoEsp32BitBangWs2812Method > NeoPixelRgbw;
#elif defined(CONFIG_IDF_TARGET_ESP32C3)
    typedef NeoPixelBus<DotStarBgrFeature, DotStarSpi10MhzMethod > DotStar;
    typedef NeoPixelBus<NeoGrbFeature, NeoEsp32Rmt0Ws2812xMethod > NeoPixel;
    typedef NeoPixelBus<NeoGrbwFeature, NeoEsp32Rmt0Sk6812Method > NeoPixelRgbw;
#elif defined(CONFIG_IDF_TARGET_ESP32S3)
    typedef NeoPixelBus<DotStarBgrFeature, DotStarSpi10MhzMethod > DotStar;
    typedef NeoPixelBus<NeoGrbFeature, NeoEsp32LcdX8Ws2812Method > NeoPixel;
    typedef NeoPixelBus<NeoGrbwFeature, NeoEsp32LcdX8Sk6812Method > NeoPixelRgbw;    
#elif defined(CONFIG_IDF_TARGET_ESP32S2)
    typedef NeoPixelBus<DotStarBgrFeature, DotStarSpi10MhzMethod > DotStar;
    typedef NeoPixelBus<NeoGrbFeature, NeoEsp32I2s0Ws2812xMethod> NeoPixel;
    typedef NeoPixelBus<NeoGrbwFeature, NeoEsp32I2s0Sk6812Method> NeoPixelRgbw;
#elif defined(ARDUINO_ARCH_ESP32)
    #if defined(ETH_PHY_TYPE) && (ETH_PHY_TYPE == ETH_PHY_LAN8720)
        typedef NeoPixelBus<DotStarBgrFeature, DotStarMethod> DotStar;
    #else
        typedef NeoPixelBus<DotStarBgrFeature, DotStarSpi10MhzMethod > DotStar;
    #endif
    typedef NeoPixelBus<NeoGrbFeature, NeoEsp32I2s1Ws2812xMethod> NeoPixel;
    typedef NeoPixelBus<NeoGrbwFeature, NeoEsp32I2s1Sk6812Method> NeoPixelRgbw;
#elif defined(ARDUINO_ARCH_ESP8266)
    typedef NeoPixelBus<DotStarBgrFeature, DotStarSpiMethod> DotStar;
    typedef NeoPixelBus<NeoGrbFeature, NeoEsp8266Uart1Ws2812xMethod> NeoPixel;
    typedef NeoPixelBus<NeoGrbwFeature, NeoEsp8266Uart1Sk6812Method> NeoPixelRgbw;
#else // Raspberry Pi Pico
    typedef NeoPixelBus<DotStarBgrFeature, DotStarSpi10MhzMethod> DotStar;
    typedef NeoPixelBus<NeoGrbFeature, Rp2040x4Pio0Ws2812xMethod> NeoPixel;
    typedef NeoPixelBus<NeoGrbwFeature, Rp2040x4Pio0Ws2812xMethod> NeoPixelRgbw;
#endif

struct neopixelbus_bridge : public led_bridge
{
    DotStar* dotstar = nullptr;
    NeoPixel* neopixel = nullptr;
    NeoPixelRgbw* neopixelRgbw = nullptr;

    int getLedsNumber() override
    {
        if (dotstar != nullptr) 
            return dotstar->PixelCount();
        else if (neopixel != nullptr)
            return neopixel->PixelCount();
        else if (neopixelRgbw != nullptr)
            return neopixelRgbw->PixelCount();
        
        return 0;
    }

    void clearAll() override
    {
        if (dotstar == nullptr && neopixel == nullptr && neopixelRgbw == nullptr) 
            return;

        if (dotstar != nullptr) 
            {dotstar->ClearTo(RgbColor(0, 0, 0)); dotstar->Show();}
        else if (neopixel != nullptr)
            {neopixel->ClearTo(RgbColor(0, 0, 0)); neopixel->Show();}
        else if (neopixelRgbw != nullptr)
            {neopixelRgbw->ClearTo(RgbwColor(0, 0, 0, 0)); neopixelRgbw->Show();}
    }

    bool canRender() override
    {
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
        return true;
    }
    
    bool executeRenderLed(bool isNewFrame) override
    {
        if (dotstar != nullptr) 
        {
            if (!dotstar->CanShow())
            {
                return false;
            }
            dotstar->Show();
        }
        else if (neopixel != nullptr)
        {
            if (!neopixel->CanShow())
            {
                return false;
            }
            neopixel->Show();
        }
        else if (neopixelRgbw != nullptr)
        {
            if (!neopixelRgbw->CanShow())
            {
                return false;
            }
            neopixelRgbw->Show();
        }
        return true;
    }

    void releaseDriverResources() override
    {
        delay(50);
        if (dotstar != nullptr) {delete dotstar; dotstar = nullptr;}
        if (neopixel != nullptr) {delete neopixel; neopixel = nullptr;}
        if (neopixelRgbw != nullptr) {delete neopixelRgbw; neopixelRgbw = nullptr;}
        delay(100);
    }

    void initializeLedDriver(LedType cfgLedType, uint16_t cfgLedNumLeds, uint8_t cfgLedDataPin, uint8_t cfgLedClockPin,
                            uint8_t calGain, uint8_t calRed, uint8_t calGreen, uint8_t calBlue) override
    {
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
            dotstar = new DotStar(cfgLedNumLeds, cfgLedClockPin, cfgLedDataPin);
            dotstar->Begin();
        }        
    }

    inline void setLedRgb(int index, uint8_t r, uint8_t g, uint8_t b) override
    {
        if (dotstar != nullptr)
        {
            RgbColor col(r, g, b);
            dotstar->SetPixelColor(index, col);
        }
        else if (neopixel != nullptr)
        {
            RgbColor col(r, g, b);
            neopixel->SetPixelColor(index, col);
        }
        else if (neopixelRgbw != nullptr)
        {
            ColorRgbw col = rgb2rgbw(r, g, b);
            neopixelRgbw->SetPixelColor(index, RgbwColor(col.R, col.G, col.B, col.W));
        }
    }

    inline void setLedRgbw(int index, uint8_t r, uint8_t g, uint8_t b, uint8_t w) override
    {
        if (dotstar != nullptr)
        {
            RgbColor col(r, g, b);
            dotstar->SetPixelColor(index, col);
        }
        else if (neopixel != nullptr)
        {
            RgbColor col(r, g, b);
            neopixel->SetPixelColor(index, col);
        }
        else if (neopixelRgbw != nullptr)
        {
            RgbwColor col(r, g, b, w);
            neopixelRgbw->SetPixelColor(index, col);
        }
    }
};