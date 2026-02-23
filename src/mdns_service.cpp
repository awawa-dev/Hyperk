/* mdns_service.cpp
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
#else
    #include <WiFi.h>
    #include <LEAmDNS.h>
#endif

#include "config.h"
#include "mdns_service.h"
#include "utils.h"

 namespace Mdns{
    bool mdnsInitialized = false;
    String deviceShortMacAddress = "";

    String sanitizeMdnsService(String s) {
        s.toLowerCase();
        
        char buf[16] = {0};
        int j = 0;
        bool lastDash = false;
        
        for (unsigned int i = 0; i < s.length() && j < 15; i++) {
            char c = s[i];
            if ((c >= 'a' && c <= 'z') || (c >= '0' && c <= '9')) {
                buf[j++] = c;
                lastDash = false;
            } else if (!lastDash && j > 0) {
                buf[j++] = '-';
                lastDash = true;
            }
        }
        
        if (lastDash) j--;
        buf[j] = '\0';


        if (j > 0)
        {
            Log::debug("Validated MDNS service name: ", buf);
            return String(buf);
        }
        else
        {
            Log::debug("MDNS service name is invalid: ", s);
            return String("");
        }
    }

    String getDeviceShortMacAddress() {

        if (deviceShortMacAddress.isEmpty())
        {
            deviceShortMacAddress = WiFi.macAddress();
            deviceShortMacAddress.replace(":", "");
            deviceShortMacAddress.toLowerCase();
        }

        return deviceShortMacAddress;
    }


    void startMDNS()
    {
        const AppConfig& cfg = Config::cfg;

        endMDNS();

        if (cfg.deviceName.isEmpty()) {
            Log::debug("Skipping mDNS initialization: device name is empty");
            return;
        }

        if (mdnsInitialized = MDNS.begin(cfg.deviceName.c_str()); mdnsInitialized)
        {
            String macId = getDeviceShortMacAddress();

            // 1. Extra mdns tag
            if (cfg.extraMdnsTag.length() > 0)
            {
                auto extraMDNS = sanitizeMdnsService(cfg.extraMdnsTag);
                MDNS.addService(extraMDNS, "tcp", 80);
                MDNS.addServiceTxt(extraMDNS, "tcp", "id", macId.c_str());
                String fullModelName = String(APP_NAME) + "-" + String(APP_VERSION);
                MDNS.addServiceTxt(extraMDNS, "tcp", "mdl", fullModelName.c_str());            
                MDNS.addServiceTxt(extraMDNS, "tcp", "mac", WiFi.macAddress().c_str());
                MDNS.addServiceTxt(extraMDNS, "tcp", "src", "udp");
            }

            // 2. Hyperk services
            auto ourMDNS = sanitizeMdnsService(APP_NAME);
            MDNS.addService(ourMDNS, "tcp", 80);        
            MDNS.addServiceTxt(ourMDNS, "tcp", "ver", APP_VERSION);
            MDNS.addServiceTxt(ourMDNS, "tcp", "id", macId.c_str());

            MDNS.addService("ddp", "udp", 4048);
            MDNS.addService("realtime", "udp", 21324);
            MDNS.addService("raw", "udp", 5568);

            Log::debug("mDNS: ", cfg.deviceName, ".local (", APP_NAME, " v", APP_VERSION, ")");
        }
    }

    void endMDNS() {
        if (mdnsInitialized) {
            mdnsInitialized = false;
            MDNS.end();        
        }
    }

}
