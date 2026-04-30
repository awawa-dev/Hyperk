/* config.cpp
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

#include "config.h"
#include "storage.h"
#include "volatile_state.h"

#include <algorithm>

namespace Config {
    AppConfig internalCfg;
    const AppConfig& cfg = internalCfg;

    bool loadConfig() {
        return Storage::loadConfig(internalCfg);
    }

    bool saveConfig(const AppConfig& cfg) {
        internalCfg = cfg;
        return Storage::saveConfig(internalCfg);
    }   
};

void LedConfig::deserializeSegments(const JsonArray& jsonArray) {
    segments.clear();
    
    for (JsonVariant value : jsonArray) {
        Segment seg;
        
        seg.data = value["data"] | 2;
        seg.clock = value["clock"] | 4;
        seg.startIndex = value["startIndex"] | 0;

        Log::debug("Segments restored. Data: ", seg.data, ", clock: ", seg.clock, ", start: ", seg.startIndex); 
        
        segments.push_back(seg);
    }
};

bool LedConfig::deserializeSegments(const String& rawValues) {
    std::vector<Segment> newSegments;
    const char* p = rawValues.c_str();
    char* end;
    long buffer[3];
    uint8_t count = 0;

    while (*p != '\0') {
        buffer[count++] = strtol(p, &end, 10);
        if (count == 3) {
            newSegments.push_back({
                static_cast<uint8_t>(std::clamp(buffer[0], 0l, 64l)),
                static_cast<uint8_t>(std::clamp(buffer[1], 0l, 64l)),
                static_cast<uint16_t>(std::clamp(buffer[2], 0l, 2048l))
            });
            count = 0;
        }

        p = end;
        
        if (*p == ',') {
            p++;
        }
        else {            
            break; 
        }
    };

    bool changed = (segments != newSegments) && newSegments.size() > 0;

    Log::debug("Segments changed: ", changed, ", values: ", rawValues, ", detected segments: ", newSegments.size());    

    if (changed) {
        segments = std::move(newSegments);
    }

    return changed;
};

void LedConfig::serializeSegments(JsonArray& jsonArray) const {
    for (const auto& seg : segments) {
        JsonObject obj = jsonArray.add<JsonObject>();
        
        obj["data"] = seg.data;
        obj["clock"] = seg.clock;
        obj["startIndex"] = seg.startIndex;
    }
};