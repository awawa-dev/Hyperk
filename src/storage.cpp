/* storage.cpp
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

#include "storage.h"
#include <LittleFS.h>

namespace Storage {

    bool loadConfig(AppConfig& cfg) {
        if (!LittleFS.exists(CONFIG_FILE)) {
            Log::debug("No config file → creating default");
            return saveConfig(cfg);
        }

        File file = LittleFS.open(CONFIG_FILE, "r");
        JsonDocument doc;
        DeserializationError err = deserializeJson(doc, file);
        file.close();

        if (err) {            
            LittleFS.format();
            LittleFS.begin();
            Log::debug("JSON parse error → creating default");
            return saveConfig(cfg);
        }

        cfg.wifi.ssid     = doc["wifi"]["ssid"] | "";
        cfg.wifi.password = doc["wifi"]["password"] | "";
        cfg.deviceName    = doc["deviceName"] | APP_NAME;
        cfg.extraMdnsTag  = doc["extraMdnsTag"] | "wled";

        cfg.led.type       = static_cast<LedType>(doc["led"]["type"] | static_cast<int>(LedType::WS2812));

        if (JsonArray jsonSegments = doc["segments"].as<JsonArray>(); !jsonSegments.isNull() && jsonSegments.size() > 0) {
            cfg.led.deserializeSegments(jsonSegments);
        }
        else {
            Log::debug("Cannot find segments");
            LedConfig::Segment seg;
            seg.data    = doc["led"]["dataPin"]    | 2;
            seg.clock   = doc["led"]["clockPin"]   | 4;
            seg.startIndex = 0;
            cfg.led.segments.clear();
            cfg.led.segments.push_back(seg);
        }

        JsonObject jsonRelay = doc["relay"].as<JsonObject>();
        cfg.led.relay.gpio = jsonRelay["relay-gpio"] | POWER_RELAY_GPIO;
        cfg.led.relay.inverted = jsonRelay["relay-inverted"] | POWER_RELAY_INVERT_BOOL;

        cfg.led.numLeds    = doc["led"]["numLeds"]    | 16;
        cfg.led.brightness = doc["led"]["brightness"] | 255;
        cfg.led.r          = doc["led"]["r"] | 196;
        cfg.led.g          = doc["led"]["g"] | 32;
        cfg.led.b          = doc["led"]["b"] | 8;
        cfg.led.effect     = doc["led"]["effect"] | 0;

        cfg.led.calibration.gain  = doc["calibration"]["gain"]  | 0xFF;
        cfg.led.calibration.red   = doc["calibration"]["red"]   | 0xA0;
        cfg.led.calibration.green = doc["calibration"]["green"] | 0xA0;
        cfg.led.calibration.blue  = doc["calibration"]["blue"]  | 0xA0;

        return true;
    }

    bool saveConfig(const AppConfig& cfg) {
        JsonDocument doc;

        doc["wifi"]["ssid"]     = cfg.wifi.ssid;
        doc["wifi"]["password"] = cfg.wifi.password;
        doc["deviceName"]       = cfg.deviceName;
        doc["extraMdnsTag"]        = cfg.extraMdnsTag;

        doc["led"]["type"]       = static_cast<uint8_t>(cfg.led.type);

        JsonArray segArray = doc["segments"].to<JsonArray>();
        cfg.led.serializeSegments(segArray);

        JsonObject jsonRelay = doc["relay"].to<JsonObject>();
        jsonRelay["relay-gpio"]     = cfg.led.relay.gpio;
        jsonRelay["relay-inverted"] = cfg.led.relay.inverted;

        doc["led"]["numLeds"]    = cfg.led.numLeds;
        doc["led"]["brightness"] = cfg.led.brightness;
        doc["led"]["r"]          = cfg.led.r;
        doc["led"]["g"]          = cfg.led.g;
        doc["led"]["b"]          = cfg.led.b;
        doc["led"]["effect"]     = cfg.led.effect;

        doc["calibration"]["gain"]  = cfg.led.calibration.gain;
        doc["calibration"]["red"]   = cfg.led.calibration.red;
        doc["calibration"]["green"] = cfg.led.calibration.green;
        doc["calibration"]["blue"]  = cfg.led.calibration.blue;

        File file = LittleFS.open(CONFIG_FILE, "w");
        if (!file) return false;
        serializeJson(doc, file);
        file.flush();
        file.close();

        #ifdef DEBUG_LOG
            String output;
            serializeJson(doc, output);
            Log::debug("Saving config: ", output);
        #endif

        return true;
    }
};