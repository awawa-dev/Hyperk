/* manager.cpp
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

#include <Arduino.h>
#include "utils.h"
#include "config.h"
#include "leds.h"
#include "manager.h"
#include "mdns_service.h"
#include "volatile_state.h"

Stats stats;

namespace
{
    volatile bool _pendingApplyConfig = false;
    volatile bool _pendingReboot = false;
    unsigned long _rebootTime = 0;
}

void managerScheduleApplyConfig()
{
    _pendingApplyConfig = true;
}

void managerScheduleReboot(uint32_t delay_ms)
{
    _rebootTime = millis() + delay_ms;
    _pendingReboot = true;

    Log::debug("--- REBOOT SCHEDULED ---");
    Log::debug("Current millis: ", millis());
    Log::debug("Reboot target : ", _rebootTime);
}

void managerLoop()
{
    Volatile::checkStreamTimeout();

    Leds::synchronizeLedsToVolatileStateBeforeDeleyedRender();
    Leds::checkDeleyedRender();

    if (_pendingReboot)
    {
        if (millis() >= _rebootTime) {
            rebootDevice();
        }
        else {
            Mdns::endMDNS();
        }
    }

    if (_pendingApplyConfig && !_pendingReboot)
    {
        _pendingApplyConfig = false;
        Leds::applyLedConfig();    
    }

    uint32_t currentTag = millis() / 1000;   
    if (currentTag != stats.currentTag) 
    {
        stats.lastIntervalRenderedFrames = stats.renderedFrames;
        stats.lastIntervalSkippedFrames = stats.skippedFrames;
        stats.renderedFrames = 0;
        stats.skippedFrames = 0;
        stats.currentTag = currentTag;
    }
}