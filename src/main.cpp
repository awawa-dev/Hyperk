/* main.cpp
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

#if defined(ARDUINO_ARCH_ESP8266)
    #include <ESP8266WiFi.h>
    #include <ESP8266mDNS.h>
#elif defined(ARDUINO_ARCH_ESP32)
    #include <WiFi.h>
    #include <ESPmDNS.h>
    #ifdef WEBSERVER_USE_ETHERNET
        #include <ETH.h>
    #endif
#elif defined(ARDUINO_ARCH_RP2040) || defined(ARDUINO_ARCH_RP2350)
    #include <WiFi.h>
    #include <LEAmDNS.h>
#endif

#include <ESPAsyncWebServer.h>
#include <LittleFS.h>
#include <WiFiUdp.h>
#include <DNSServer.h>
#include "config.h"
#include "storage.h"
#include "mdns_service.h"
#include "web_server.h"
#include "udp_receiver.h"
#include "leds.h"
#include "manager.h"

namespace {
    DNSServer dnsServer;
    AsyncWebServer server(80);
    WiFiUDP udpDDP, udpRealTime, udpRAW;

    bool inAPMode = false;
    bool hasEthernet = false; 
}

void startAP() {
    inAPMode = true;
    WiFi.mode(WIFI_AP);
    WiFi.softAP(APP_NAME "-Setup");    
    delay(200);     
    IPAddress IP = WiFi.softAPIP();    
    dnsServer.setErrorReplyCode(DNSReplyCode::NoError);
    dnsServer.start(53, "*", IP);        
    Log::debug("Captive Portal Ready at: ", IP);
}

bool isAPMode() {
    return inAPMode;
}

void setup() {
    Serial.begin(115200);
    delay(500);

    #ifdef DEBUG_LOG
        delay(8000);
    #endif

    #if defined(ARDUINO_ARCH_ESP32)
        LittleFS.begin(true);
    #else
        if (!LittleFS.begin()) {
            Log::debug("FS Mount failed, formatting...");
            LittleFS.format();
            LittleFS.begin();
        }
    #endif

    Config::loadConfig();

    Leds::applyLedConfig();

    #ifdef WEBSERVER_USE_ETHERNET
        ETH.begin();

        unsigned long timeout = millis() + 8000;
        while (millis() < timeout) {
            if (auto localIp = ETH.localIP(); localIp != IPAddress(0, 0, 0, 0)) {
                Log::debug("Ethernet Connected → ", localIp.toString());
                hasEthernet = true;
                break;
            }
            if (millis() > (timeout - 5000) && !ETH.linkUp()) {
                Log::debug("The cable is disconnected. Give up waiting for ethernet connection.");
                break;
            }
            delay(500);
            Log::debug(".");    
        }

        if (!hasEthernet) {
            Log::debug("Starting WiFi Fallback...");
        }
    #endif
    
    const AppConfig& cfg = Config::cfg;

    // WiFi connection with fallback
    if (!hasEthernet){
        if (cfg.wifi.ssid.length() > 0)
        {
            WiFi.begin(cfg.wifi.ssid.c_str(), cfg.wifi.password.c_str());
            uint32_t timeout = millis() + 12000;
            while (WiFi.status() != WL_CONNECTED && millis() < timeout)
            {
                delay(400);
            }
        }

        if (WiFi.status() != WL_CONNECTED)
        {
            startAP();
        }
        else
        {
            Log::debug("Connected → ", WiFi.localIP());
        }
    }

    Mdns::startMDNS();
    setupWebServer(server);
    server.begin();

    Log::debug("HTTP Server started");

    udpDDP.begin(4048);
    Log::debug("UDP DDP listener started on port 4048");
    udpRealTime.begin(21324);
    Log::debug("UDP RealTime listener started on port 21324");
    udpRAW.begin(5568);
    Log::debug("Raw RGB color stream listener started on port 5568");
}

void loop()
{
    handleDDP(udpDDP);
    handleRealTime(udpRealTime);
    handleRAW(udpRAW);    
    managerLoop();

    // external libraries
    if (inAPMode)
    {
        dnsServer.processNextRequest();
    }
    #if !defined(ARDUINO_ARCH_ESP32)
        MDNS.update();
    #endif


    #if defined(DEBUG_LOG) && (defined(ESP32) || defined(ESP8266))
        static uint32_t lastAlive = 0;
        if (millis() - lastAlive > 1000) {
            lastAlive = millis();
            uint32_t log1 = 0, log2 = 0;
            #if defined(ESP32)
                log1 = heap_caps_get_largest_free_block(MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);
                log2 = heap_caps_get_minimum_free_size(MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);
            #else
                log1 = ESP.getMaxFreeBlockSize();
                log2 = ESP.getHeapFragmentation();
            #endif
            Serial.printf("[MEM_REPORT %lu] Heap: %6u | Largest block: %6u | Min free/frag: %6u\n", millis(), ESP.getFreeHeap(), log1, log2);
        }
    #endif    
}