/* calibration.cpp
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

#if !defined(USE_FASTLED)
    #include <NeoPixelBus.h>
#endif

#include "leds.h"
#include "calibration.h"

/* Following code comes from HyperSerialEsp8266 and HyperSerialESP32 as I invented this calibration procedure for RGBW LEDs
*
*  HyperSerialEsp8266/HyperSerialESP32
*  MIT License
*
*  Copyright (c) 2020-2026 awawa-dev
*
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

#define ROUND_DIVIDE(numer, denom) (((numer) + (denom) / 2) / (denom))

namespace
{
    struct ChannelCorrection
    {
        uint8_t white[256];
        uint8_t red[256];
        uint8_t green[256];
        uint8_t blue[256];
    };

    // calibration parameters
    uint8_t gain = 0xFF;
    uint8_t red = 0xA0;
    uint8_t green = 0xA0;
    uint8_t blue = 0xA0;

    ChannelCorrection* _data = nullptr;
    #define channelCorrection (*_data)
}

void printCalibration()
{
    Log::debug("RGBW => Gain: ",gain ,"/255, red: ",red ,", green: ",green ,", blue: ", blue);
}

void deleteCalibration()
{
    if (_data != nullptr)
    {
        delete _data;
        _data = nullptr;
    }
}

void prepareCalibration()
{
    if (_data == nullptr)
    {
        _data = new ChannelCorrection();
    }

    // prepare LUT calibration table, cold white is much better than "neutral" white
    for (uint32_t i = 0; i < 256; i++)
    {
        // color calibration
        uint32_t _gain = gain * i;   // adjust gain
        uint32_t _red = red * i;     // adjust red
        uint32_t _green = green * i; // adjust green
        uint32_t _blue = blue * i;   // adjust blue

        channelCorrection.white[i] = (uint8_t)min(ROUND_DIVIDE(_gain, 0xFF), (uint32_t)0xFF);
        channelCorrection.red[i]   = (uint8_t)min(ROUND_DIVIDE(_red,  0xFF), (uint32_t)0xFF);
        channelCorrection.green[i] = (uint8_t)min(ROUND_DIVIDE(_green,0xFF), (uint32_t)0xFF);
        channelCorrection.blue[i]  = (uint8_t)min(ROUND_DIVIDE(_blue, 0xFF), (uint32_t)0xFF);
    }
}

void setParamsAndPrepareCalibration(uint8_t _gain, uint8_t _red, uint8_t _green, uint8_t _blue)
{
    if (gain != _gain || red != _red || green != _green || blue != _blue || _data == nullptr)
    {
        gain = _gain;
        red = _red;
        green = _green;
        blue = _blue;
        prepareCalibration();
        printCalibration();
    }    
}

RgbwColor rgb2rgbw(uint8_t r, uint8_t g, uint8_t b)
{
    RgbwColor color;

    color.W = min(channelCorrection.red[r],
                    min(channelCorrection.green[g],
                        channelCorrection.blue[b]));
    color.R = r - channelCorrection.red[color.W];
    color.G = g - channelCorrection.green[color.W];
    color.B = b - channelCorrection.blue[color.W];
    color.W = channelCorrection.white[color.W];
    return color;
}
