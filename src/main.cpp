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

DNSServer dnsServer;
AsyncWebServer server(80);
WiFiUDP udpDDP, udpRealTime, udpRAW;
bool inAPMode = false;

void startAP() {
    inAPMode = true;
    WiFi.mode(WIFI_AP);
    WiFi.softAP(APP_NAME "-Setup");    
    delay(200);     
    IPAddress IP = WiFi.softAPIP();    
    dnsServer.setErrorReplyCode(DNSReplyCode::NoError);
    dnsServer.start(53, "*", IP);        
    Serial.printf("Captive Portal Ready at: %s\n", IP.toString().c_str());
}

bool isAPMode() {
    return inAPMode;
}


void setup() {
    Serial.begin(115200);
    delay(500);

    #if defined(ARDUINO_ARCH_ESP32)
        LittleFS.begin(true);
    #else
        if (!LittleFS.begin()) {
            Serial.println("FS Mount failed, formatting...");
            LittleFS.format();
            LittleFS.begin();
        }
    #endif

    loadConfig();

    applyLedConfig();

    // WiFi connection with fallback
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
        Serial.printf("Connected → %s\n", WiFi.localIP().toString().c_str());
    }

    startMDNS();
    setupWebServer(server);
    server.begin();

    Serial.print("HTTP Server started on IP: ");
    Serial.println(WiFi.localIP());

    udpDDP.begin(4048);
    Serial.println("UDP DDP listener started on port 4048");
    udpRealTime.begin(21324);
    Serial.println("UDP RealTime listener started on port 21324");
    udpRAW.begin(5568);
    Serial.println("Raw RGB color stream listener started on port 5568");
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
}