/* volatile_state.cpp
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
#include "volatile_state.h"

namespace Volatile{
    static VolatileState internalState;
    const VolatileState& state = internalState;

    bool updatedBrightness = false;
    bool updatedPowerOn = false;
    bool updatedStaticColor = false;

    void updateBrightness(uint8_t  brightness){
        internalState.brightness = brightness;
        updatedBrightness = true;
    };

    void updatePowerOn(bool on){
        internalState.on = on;
        updatedPowerOn = true;
        
        updateStreamTimeout(0);
    };

    void updateStaticColor(uint8_t red, uint8_t green, uint8_t blue){
        internalState.staticColor.red = red;
        internalState.staticColor.green = green;
        internalState.staticColor.blue = blue;
        updatedStaticColor = true;

        updateStreamTimeout(0);
    };

    void updateStreamTimeout(unsigned long timeout){
        internalState.streamTimeout = (timeout) ? timeout + millis() : 0;
        internalState.live = (timeout);
    };

    void checkStreamTimeout(){
        if (internalState.streamTimeout > 0 && internalState.streamTimeout < millis())
        {
            updatePowerOn(false);

            internalState.streamTimeout = 0;
            internalState.live = false;
        }        
    };    

    bool clearUpdatedBrightnessState(){
        bool ret = updatedBrightness;
        updatedBrightness = false;
        return ret;
    };

    bool clearUpdatedPowerOnState(){
        bool ret = updatedPowerOn;
        updatedPowerOn = false;
        return ret;
    };

    bool clearUpdatedStaticColorState(){
        bool ret = updatedStaticColor;
        updatedStaticColor = false;
        return ret;
    };
}

