// File: include/config.h
#pragma once
#include <Arduino.h>
#include <ArduinoJson.h>
#include <vector>
#include "logger.h"

#define CONFIG_FILE "/config.json"

#ifndef POWER_RELAY_GPIO
    #define POWER_RELAY_GPIO -1 
#endif

#ifdef POWER_RELAY_INVERT
    #define POWER_RELAY_INVERT_BOOL true
#else
    #define POWER_RELAY_INVERT_BOOL false
#endif

enum class LedType : uint8_t {
    WS2812 = 0,
    SK6812 = 1,
    APA102 = 2
};

struct LedConfig {
    struct Segment {
        uint8_t data;
        uint8_t clock;
        uint16_t startIndex;

        bool operator==(const Segment& other) const {
            return data == other.data && clock == other.clock && startIndex == other.startIndex;
        }

        bool operator!=(const Segment& other) const {
            return !(*this == other);
        }
    };

    struct Relay {
        int8_t gpio;
        bool inverted;
    };

    LedType  type       = LedType::WS2812;
    std::vector<Segment> segments = {{2, 4, 0}};
    Relay  relay = {POWER_RELAY_GPIO, POWER_RELAY_INVERT_BOOL};
    uint16_t numLeds    = 16;
    uint8_t  brightness = 255;
    uint8_t  r = 196, g = 32, b = 8;
    uint8_t  effect     = 0;

    struct Calibration {
	    uint8_t gain  = 0xFF;
	    uint8_t red   = 0xA0;
	    uint8_t green = 0xA0;
	    uint8_t blue  = 0xA0;
    } calibration;

    void deserializeSegments(const JsonArray& jsonArray);
    bool deserializeSegments(const String& rawValues);
    void serializeSegments(JsonArray& jsonArray) const;
};

struct AppConfig {
    struct WifiConfig {
        String ssid;
        String password;
    } wifi;
    LedConfig  led;
    String     deviceName = "hyperk";
    String     extraMdnsTag = "wled";
};

namespace Config {
    extern const AppConfig& cfg;

    bool loadConfig();
    bool saveConfig(const AppConfig &cfg);
};
