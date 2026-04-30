/* picolada_bridge.h
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

#include "picolada/picolada.h"
#include "led_bridge.h"

struct picolada_bridge : public led_bridge
{
    struct ExtSegments{
        LedConfig::Segment segment;
        sk6812* singleSk6812 = nullptr;
        ws2812* singleWs2812 = nullptr;
        sk6812p* multiSk6812 = nullptr;
        ws2812p* multiWs2812 = nullptr;
        int ledCount = 0;
    };
    std::vector<ExtSegments> _segments;

    uint16_t _totalLedsNumber = 0;
    LedType _ledsType = LedType::WS2812;    

    apa102* singleApa102 = nullptr;   

    int getLedsNumber() override
    {
        return _totalLedsNumber;
    }

    void clearAll() override
    {
        if (_segments.size() || singleApa102 != nullptr)
        {
            for (int i = 0; !canRender() && i < 200; i++) {
                Log::debug("-");
                delay(1);
            }

            for (int i = 0; i < _totalLedsNumber; i++) {
                setLedRgb(i, 0 ,0, 0);
            }
            executeRenderLed();

            for (int i = 0; !canRender() && i < 200; i++) {
                Log::debug("+");
                delay(1);                
            }

            Log::debug("leds cleared");
        }
    }

    bool canRender() override
    {
        if (_segments.size())
        {
            auto& seg = _segments.front();

            if ((seg).singleSk6812 != nullptr)      { return (seg).singleSk6812->isReady(); } 
            else if ((seg).singleWs2812 != nullptr) { return (seg).singleWs2812->isReady(); }
            else if ((seg).multiSk6812 != nullptr)  { return (seg).multiSk6812->isReady(); }
            else if ((seg).multiWs2812 != nullptr)  { return (seg).multiWs2812->isReady(); }
        }
        else if (singleApa102)
        {
            return singleApa102->isReady();
        }
        return true;
    }

    int segmentSupported() override
    {
        return 8;
    }

    bool supportsDoubleBuffering() override {
        return true;
    }    

    void executeRenderLed() override
    {
        if (_segments.size())
        {
            auto& seg = _segments.front();

            if ((seg).singleSk6812 != nullptr)      { (seg).singleSk6812->renderSingleLane(); }
            else if ((seg).singleWs2812 != nullptr) { (seg).singleWs2812->renderSingleLane(); }
            else if ((seg).multiSk6812 != nullptr)  { (seg).multiSk6812->renderAllLanes(); }
            else if ((seg).multiWs2812 != nullptr)  { (seg).multiWs2812->renderAllLanes(); }
        }
        else if (singleApa102)
        {
            singleApa102->renderSingleLane();
        }
    }

    void releaseDriverResources() override
    {        
        delay(50);

        for(auto& seg : _segments){
            delete seg.singleSk6812; seg.singleSk6812 = nullptr;
            delete seg.singleWs2812; seg.singleWs2812 = nullptr;
            delete seg.multiSk6812; seg.multiSk6812 = nullptr;
            delete seg.multiWs2812; seg.multiWs2812 = nullptr;
        }
        _segments.clear();

        delete singleApa102; singleApa102 = nullptr;
        
        delay(50);
    }

    void initializeLedDriver(LedType cfgLedType, uint16_t cfgLedNumLeds, const std::vector<LedConfig::Segment>& cfgSegments,
                            uint8_t calGain, uint8_t calRed, uint8_t calGreen, uint8_t calBlue) override
    {
        if (cfgSegments.size() < 0) return;

        _totalLedsNumber = cfgLedNumLeds;
        _ledsType = cfgLedType;

        if (_ledsType == LedType::SK6812)
        {
            setParamsAndPrepareCalibration(calGain, calRed, calGreen, calBlue);
        }

        if (_ledsType == LedType::WS2812 || _ledsType == LedType::SK6812)
        {
            bool single = (cfgSegments.size() == 1);
            bool error = false;
            for(int i = 0; i < cfgSegments.size() && i < 8 && !error; i++) {
                const auto& seg = cfgSegments[i];
                
                int nextIndex = (i + 1 < cfgSegments.size()) ? cfgSegments[i + 1].startIndex : cfgLedNumLeds;
                ExtSegments newSeg{};

                newSeg.segment = seg;
                newSeg.ledCount = std::max(nextIndex - seg.startIndex, 0);

                if (_ledsType == LedType::SK6812) {
                    if (single)
                        newSeg.singleSk6812 = new sk6812(newSeg.ledCount, seg.data);
                    else
                        newSeg.multiSk6812 = new sk6812p(newSeg.ledCount, cfgSegments[0].data);
                }
                else {
                    if (single)
                        newSeg.singleWs2812 = new ws2812(newSeg.ledCount, seg.data);
                    else
                        newSeg.multiWs2812 = new ws2812p(newSeg.ledCount, cfgSegments[0].data);
                }
                
                _segments.push_back(newSeg);

                Log::debug("Created Neopixel segment for ", newSeg.ledCount, " LEDS at: ", seg.startIndex, ", GPIO: ",  ((single) ? seg.data : cfgSegments[0].data + i), 
                            ", driver: ", (newSeg.singleSk6812 != nullptr || newSeg.multiSk6812 != nullptr) ? "SK6812" : "WS2812B" );
                    
            }     
        }
        else
        { // SPI (APA102 / SK9822)
            auto& segment = cfgSegments.front();
            
            singleApa102 = new apa102(cfgLedNumLeds, spi0, segment.data, segment.clock);

            Log::debug("Created SPI segment for ", cfgLedNumLeds, ", DATA: ",  segment.data, ", CLOCK: ",  segment.clock );
        }        
    }

    inline void setLedRgb(int index, uint8_t r, uint8_t g, uint8_t b) override
    {
        if (_ledsType == LedType::SK6812)
        {            
            const ColorRgbw calibrated = rgb2rgbw(r, g, b);
            setLedRgbw(index, calibrated.R, calibrated.G, calibrated.B, calibrated.W);
        }
        else if (_ledsType == LedType::WS2812)
        {            
            for(const auto& seg : _segments) {
                if (index < seg.ledCount) {
                    if ((seg).singleWs2812 != nullptr) { ColorGrb32 c; c.R = r; c.G = g; c.B = b; (seg).singleWs2812->SetPixel(index,c); }
                    else if ((seg).multiWs2812 != nullptr) { ColorGrb c; c.R = r; c.G = g; c.B = b; (seg).multiWs2812->SetPixel(index,c); }
                    return;
                }
                else {
                    index -= seg.ledCount;
                }
            }
        }
        else if (singleApa102) {
            ColorDotstartBgr c;
            c.R = r; c.G = g; c.B = b;
            singleApa102->SetPixel(index, c);
        }
    }

    inline void setLedRgbw(int index, uint8_t r, uint8_t g, uint8_t b, uint8_t w) override
    {
        if (_ledsType == LedType::SK6812)
        {
            ColorGrbw c;
            c.R = r; c.G = g; c.B = b, c.W = w;
            for(const auto& seg : _segments) {
                if (index < seg.ledCount) {
                    if ((seg).singleSk6812 != nullptr) { (seg).singleSk6812->SetPixel(index,c); }
                    else if ((seg).multiSk6812 != nullptr) { (seg).multiSk6812->SetPixel(index,c); }
                    return;
                }
                else {
                    index -= seg.ledCount;
                }
            }
        }
        else if (_ledsType == LedType::WS2812)
        {
            setLedRgb(index, r, g, b);
        }
        else if (_ledsType == LedType::APA102)
        {
            setLedRgb(index, r, g, b);
        }
    }
};