/* utils.cpp
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
#include <LittleFS.h>

#if defined(ARDUINO_ARCH_ESP8266)
    #include <ESP8266WiFi.h>
#else
    #include <WiFi.h>
#endif

#if defined(ARDUINO_ARCH_RP2040) || defined(ARDUINO_ARCH_RP2350)
    #include "hardware/watchdog.h"
#endif

#include "utils.h"
#include "logger.h"

void rebootDevice() {
    Log::debug("[UTILS] Critical: Hyperk is rebooting system...");
    
    WiFi.disconnect(true);
    Log::debug("[UTILS] WiFi is closed.");
        
    LittleFS.end(); 
    Log::debug("[UTILS] File System unmounted.");

    Log::debug("[UTILS] Executing Hardware Reset...");
    delay(200);

    #if defined(ARDUINO_ARCH_RP2040) || defined(ARDUINO_ARCH_RP2350)
        watchdog_reboot(0, 0, 0);
        watchdog_enable(1, 1);
    #else
        ESP.restart();
    #endif

    while (true) {
        Serial.print(".");
        delay(1);
    }
}

String getDeviceArch()
{
    String modelName;

    #if defined(ARDUINO_ARCH_RP2350) || (defined(ARDUINO_ARCH_RP2040) && defined(__ARM_ARCH_8M_MAIN__))
        modelName = F("RP2350");
    #elif defined(ARDUINO_ARCH_RP2040)
        modelName = F("RP2040");
    #elif defined(ARDUINO_ARCH_ESP32) && defined(WEBSERVER_USE_ETHERNET)
        modelName = F("ESP32-ETH01");
    #elif defined(CONFIG_IDF_TARGET_ESP32)
        modelName = F("ESP32");
    #elif defined(CONFIG_IDF_TARGET_ESP32S2)
        modelName = F("ESP32-S2");
    #elif defined(CONFIG_IDF_TARGET_ESP32S3)
        modelName = F("ESP32-S3");
    #elif defined(CONFIG_IDF_TARGET_ESP32C3)
        modelName = F("ESP32-C3");
    #elif defined(CONFIG_IDF_TARGET_ESP32C6)
        modelName = F("ESP32-C6");
    #elif defined(ESP8266)
        modelName = F("ESP8266");
    #else
        modelName = F("generic");
    #endif

    return modelName;
}

int getFreeHeap()
{
    #if defined(ARDUINO_ARCH_ESP32) || defined(ARDUINO_ARCH_ESP8266)
        return ESP.getFreeHeap();
    #else
        return rp2040.getFreeHeap();
    #endif
}