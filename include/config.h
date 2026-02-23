// File: include/config.h
#pragma once
#include <Arduino.h>
#include <ArduinoJson.h>
#include <vector>
#include "logger.h"

#define CONFIG_FILE "/config.json"

enum class LedType : uint8_t {
    WS2812 = 0,
    SK6812 = 1,
    APA102 = 2
};

struct LedConfig {
    LedType  type       = LedType::WS2812;
    uint8_t  dataPin    = 2;     // default for most boards
    uint8_t  clockPin   = 4;
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
