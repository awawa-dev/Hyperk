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
    #include <Updater.h>
#elif defined(ARDUINO_ARCH_ESP32)
    #include <WiFi.h>
    #include <ESPmDNS.h>
    #ifdef WEBSERVER_USE_ETHERNET
        #include <ETH.h>
    #endif
    #include <Update.h>
#elif defined(ARDUINO_ARCH_RP2040) || defined(ARDUINO_ARCH_RP2350)
    #include <WiFi.h>
    #include <LEAmDNS.h>
    #include <Updater.h>
#endif

#ifdef USE_ASYNC_WEBSERWER
    #include <ESPAsyncWebServer.h>
#elif defined(USE_SYNC_WEBSERWER)
    #include <WebServer.h>
#else
    #include <esp_http_server.h>
#endif

#include <LittleFS.h>
#include <WiFiUdp.h>
#include <DNSServer.h>
#include "main.h"
#include "web_resources_OSS.h"

namespace {
    DNSServer dnsServer;
    WiFiUDP udpDDP, udpRealTime, udpRAW;

    bool inAPMode = false;
    bool hasEthernet = false;
    bool scheduledApRestart = false;

    void startAP(bool scheduleAutomaticRestartToReconnect) {
        constexpr uint16_t DNS_PORT = 53;
        inAPMode = true;
        WiFi.mode(WIFI_AP);
        WiFi.softAP(APP_NAME "-Setup");    
        delay(200);     
        IPAddress IP = WiFi.softAPIP();    
        dnsServer.setErrorReplyCode(DNSReplyCode::NoError);
        dnsServer.start(DNS_PORT, "*", IP);        
        Log::SERIAL_LOG("Captive Portal Ready at: ", IP);

        if (scheduleAutomaticRestartToReconnect) {
            constexpr auto restartTime = 3 * 60000;
            Log::SERIAL_LOG("Scheduling automatic restart in ", (restartTime / 1000), " seconds to reconnect to the configured WiFi network");
            Manager::scheduleReboot(restartTime);
            scheduledApRestart = true;
        }    
    }
}

bool isAPMode() {
    return inAPMode;
}

void setup() {
    #if defined(ARDUINO_ARCH_ESP32)
        LittleFS.begin(true);
    #else
        if (!LittleFS.begin()) {
            Log::SERIAL_LOG("FS Mount failed, formatting...");
            LittleFS.format();
            LittleFS.begin();
        }
    #endif

    #ifdef ENABLE_DEBUG        
        delay(8000);
    #endif

    Config::loadConfig();

    SerialPort::init(Config::getSeriaPortSpeed());

    Leds::applyLedConfig();

    #ifdef WEBSERVER_USE_ETHERNET
        ETH.begin();

        unsigned long timeout = millis() + 8000;
        while (millis() < timeout) {
            if (auto localIp = ETH.localIP(); localIp != IPAddress(0, 0, 0, 0)) {
                Log::SERIAL_LOG("Ethernet Connected → ", localIp.toString());
                hasEthernet = true;
                break;
            }
            if (millis() > (timeout - 5000) && !ETH.linkUp()) {
                Log::SERIAL_LOG("The cable is disconnected. Give up waiting for ethernet connection.");
                break;
            }
            delay(500);
            Log::SERIAL_LOG(".");    
        }

        if (!hasEthernet) {
            Log::SERIAL_LOG("Starting WiFi Fallback...");
        }
    #endif    

    // WiFi connection with fallback
    if (!hasEthernet){
        const char *ssid = nullptr, *pass = nullptr;
        bool hasWifiConfig = Config::getWifiCredentials(ssid, pass);
        if (hasWifiConfig)
        {
            WiFi.begin(ssid, pass);
            uint32_t timeout = millis() + 12000;
            while (WiFi.status() != WL_CONNECTED && millis() < timeout)
            {
                delay(400);
            }
        }

        if (WiFi.status() != WL_CONNECTED)
        {
            startAP(hasWifiConfig);
        }
        else
        {
            Log::SERIAL_LOG("Connected → ", WiFi.localIP());
        }
    }

    if (!inAPMode) {
        Mdns::startMDNS();
    }

    WebServerProvider::setupWebServer();

    Log::SERIAL_LOG("HTTP Server started");

    if (!inAPMode) {
        udpDDP.begin(4048);
        Log::SERIAL_LOG("UDP DDP listener started on port 4048");
        udpRealTime.begin(21324);
        Log::SERIAL_LOG("UDP RealTime listener started on port 21324");
        udpRAW.begin(5568);
        Log::SERIAL_LOG("Raw RGB color stream listener started on port 5568");
    }

    (void)Update;    
}

void loop()
{    
    #if defined(SERIAL_PORT_LITE)
        SerialPort::processEventsFromLoop();
    #endif

    if (!inAPMode) {
        UdpReceiver::handleDDP(udpDDP);
        UdpReceiver::handleRealTime(udpRealTime);
        UdpReceiver::handleRAW(udpRAW);
    }

    Manager::processEvents();

    // external libraries
    if (inAPMode)
    {
        dnsServer.processNextRequest();
        if (scheduledApRestart && WiFi.softAPgetStationNum()){
            scheduledApRestart = false;
            Log::SERIAL_LOG("Client has connected to Hyperk AP. Cancelling scheduled restart.");
            Manager::cancelScheduledReboot();
        }
    }
    #if !defined(ARDUINO_ARCH_ESP32)
        if (!inAPMode) {
            MDNS.update();
        }
    #endif


    #if defined(ENABLE_DEBUG) && (defined(ESP32) || defined(ESP8266))
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
            if (auto serialRes = SerialPort::getFreeSerialPortStack(); serialRes)
                Log::SERIAL_LOG("[MEM_REPORT ", millis(), "] Heap: ", ESP.getFreeHeap()," | Largest block: ", log1," | Min free/frag: ", log2," | SerialTaskMem: ", serialRes);
            else
                Log::SERIAL_LOG("[MEM_REPORT ", millis(), "] Heap: ", ESP.getFreeHeap()," | Largest block: ", log1," | Min free/frag: ", log2);
        }
    #endif    
}