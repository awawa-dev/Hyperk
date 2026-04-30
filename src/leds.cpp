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

#include "leds.h"
#include "config.h"
#include "storage.h"
#include "manager.h"
#include "calibration.h"
#include "volatile_state.h"

//////////////////////////////////////////////////////////////////////////////////////////////////
#if defined(USE_PICOLADA)
    #include "led_bridge/picolada_bridge.h"
    namespace Leds{ picolada_bridge renderer; }
#elif defined(USE_MULTI_ESP32_LED_STRIP)
    #include "led_bridge/multi_esp32_led_strip_bridge.h"
    namespace Leds{ 
        #if defined(CONFIG_IDF_TARGET_ESP32C2)
            multi_esp32_led_strip_bridge<false> renderer;
        #else
            multi_esp32_led_strip_bridge<true> renderer;
        #endif
    }
#elif defined(USE_FASTLED)
    #include "led_bridge/fastled_bridge.h"
    namespace Leds{ fastled_bridge renderer; }
#elif defined(USE_PARLIO_LED_STRIP)
    #include "led_bridge/parlio_bridge.h"
    namespace Leds{ parlio_bridge<true> renderer; }
#elif defined(USE_ESPRESSIF_LED_STRIP)
    #include "led_bridge/espressif_bridge.h"
    namespace Leds{ espressif_bridge renderer; }
#else
    #include "led_bridge/neopixelbus_bridge.h"
    namespace Leds{ neopixelbus_bridge renderer; }
#endif

namespace Leds{
    bool ledDriverInitialized = false;
    volatile bool delayedRender = false;
    uint16_t briPlus = 256;

    bool restartRequired()
    {
        return renderer.restartRequired();
    }

    int getLedsNumber()
    {
        return renderer.getLedsNumber();
    }

    int segmentSupported()
    {
        return renderer.segmentSupported();
    }

    bool supportsDoubleBuffering()
    {
        return renderer.supportsDoubleBuffering();
    }

    void tryWaitForRenderer()
    {
        const int max_waiting = 80;
        int wait = 0;
        for (wait = 0; !renderer.canRender() && wait < max_waiting; wait++) {
            delay(1);                
        }

        if (wait > 0) {
            Log::debug("Had to wait for LED renderer: ", wait);
        }
    }

    void synchronizeLedsToVolatileStateBeforeDelayedRender()
    {
        if (delayedRender || !renderer.canRender())
            return;

        bool updated = false;
        if (Volatile::clearUpdatedBrightnessState())
        {
            updated = true;
            Log::debug("Updating brightness to: ", Volatile::state.brightness);
            briPlus = Volatile::state.brightness + 1;
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
            tryWaitForRenderer();

            auto r = (Volatile::state.on) ? Volatile::state.staticColor.red : 0;
            auto g = (Volatile::state.on) ? Volatile::state.staticColor.green : 0;
            auto b = (Volatile::state.on) ? Volatile::state.staticColor.blue : 0;

            Volatile::setRelay(r || g || b);
            
            for(int i = 0; i < getLedsNumber(); i++) {
                if (Volatile::state.brightness != 255)
                    setLed<true>(i, r, g, b);
                else
                    setLed<false>(i, r, g, b);
            }

            renderLed(true);
        }
    }

    void initLEDs(LedType cfgLedType, uint16_t cfgLedNumLeds, const std::vector<LedConfig::Segment>& cfgSegments,
                    uint8_t calGain, uint8_t calRed, uint8_t calGreen, uint8_t calBlue) {
        renderer.clearAll();

        if (renderer.restartRequired()){
            if (ledDriverInitialized)
            {
                if (cfgLedType == LedType::SK6812) {
                    setParamsAndPrepareCalibration(calGain, calRed, calGreen, calBlue);
                }
                return;
            }
        }

        renderer.releaseDriverResources();

        if (cfgLedType != LedType::SK6812)
        {
            deleteCalibration();
        }

        delayedRender = false;

        tryWaitForRenderer();

        // LED controller setup
        renderer.initializeLedDriver(cfgLedType, cfgLedNumLeds, cfgSegments, calGain, calRed, calGreen, calBlue);

        renderer.clearAll();

        ledDriverInitialized = true;
    }

    void applyLedConfig()
    {
        const AppConfig& cfg = Config::cfg;

        Volatile::setRelay(cfg.led.r || cfg.led.g || cfg.led.b);
        initLEDs(cfg.led.type, cfg.led.numLeds, cfg.led.segments, cfg.led.calibration.gain, cfg.led.calibration.red, cfg.led.calibration.green, cfg.led.calibration.blue);
        Volatile::updateBrightness(cfg.led.brightness);
        Volatile::updatePowerOn(cfg.led.r || cfg.led.g || cfg.led.b);
        Volatile::updateStaticColor(cfg.led.r, cfg.led.g, cfg.led.b);
    }

    inline uint8_t scaleBri(uint8_t v)
    {
        return (static_cast<uint16_t>(v) * briPlus) >> 8;
    }

    template<bool applyBrightness>
    void setLed(int index, uint8_t r, uint8_t g, uint8_t b)
    {
        if constexpr (applyBrightness) {
            r = scaleBri(r);
            g = scaleBri(g);
            b = scaleBri(b);
        }

        renderer.setLedRgb(index, r, g, b);
    }

    template<bool applyBrightness>
    void setLedW(int index, uint8_t r, uint8_t g, uint8_t b, uint8_t w)
    {
        if constexpr (applyBrightness) {
            r = scaleBri(r);
            g = scaleBri(g);
            b = scaleBri(b);
            w = scaleBri(w);
        }

        renderer.setLedRgbw(index, r, g, b, w);
    }

    void checkDelayedRender()
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
        if (!renderer.canRender())
        {
            queueRender(isNewFrame);
        }
        else
        {
            renderer.executeRenderLed();
            delayedRender = false;
            stats.renderedFrames = stats.renderedFrames + 1;
        }
    }

    template void setLed<false>(int, uint8_t, uint8_t, uint8_t);
    template void setLedW<false>(int, uint8_t, uint8_t, uint8_t, uint8_t);    
    template void setLed<true>(int, uint8_t, uint8_t, uint8_t);
    template void setLedW<true>(int, uint8_t, uint8_t, uint8_t, uint8_t);    
}