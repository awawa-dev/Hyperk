/* web_server.cpp
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
#if defined(ESP8266)
    #include <ESP8266WiFi.h>
#else
    #include <WiFi.h>
#endif

#include "web_server.h"
#include "main.h"
#include "config.h"
#include "utils.h"
#include "leds.h"
#include "manager.h"
#include "mdns_service.h"
#include "web_resources.h"

namespace {
    constexpr auto mime_application_json = "application/json";
    constexpr auto mime_text_html = "text/html";
    constexpr auto mime_text_plain = "text/plain";
}

void setupStaticHandlers(AsyncWebServer& server);

void dummyApi(JsonDocument& doc, bool onlyState)
{
    // 1. STATE
    JsonObject state = onlyState ? doc.to<JsonObject>() : doc["state"].to<JsonObject>();
    state["on"]  = true;
    state["bri"] = cfg.led.brightness;
    state["mainseg"] = 0;
    state["live"] = true;

    if (onlyState)
        return;

    // 2. INFO
    JsonObject info = doc["info"].to<JsonObject>();
    info["ver"]  = F("0.14.4");
    info["vid"]  = 2310130;
    info["leds"]["count"] = getLedsNumber();
    info["name"] = cfg.deviceName;
    info["product"] = F(APP_NAME);
    info["uptime"] = millis() / 1000;
    info["arch"] = getDeviceArch();

    // 2. WIFI
    JsonObject wifi = info["wifi"].to<JsonObject>();
    int32_t rssi = WiFi.RSSI();
    wifi["rssi"] = rssi;
    wifi["signal"] = (rssi <= -100) ? 0 : (rssi >= -50) ? 100 : 2 * (rssi + 100);
    wifi["channel"] = WiFi.channel();        

    // 3. FS
    size_t totalBytes = 0, usedBytes = 0;
    #if defined(ESP32)
        totalBytes = LittleFS.totalBytes();
        usedBytes = LittleFS.usedBytes();
    #else
        FSInfo fs_info;
        if (LittleFS.info(fs_info)) {
            totalBytes = fs_info.totalBytes;
            usedBytes = fs_info.usedBytes;
        }
    #endif
    JsonObject fs = info["fs"].to<JsonObject>();
    fs["t"] = (totalBytes > 0) ? totalBytes / 1024 : 1024; 
    fs["u"] = usedBytes / 1024;
    fs["pmt"] = 0;

    // 4. EFFECTS & PALETTES
    doc["effects"].add("Solid");
    doc["palettes"].add("Default");
}

void setupWebServer(AsyncWebServer& server) {
    // minimal dummy JSON API for HA discovery and telemetry
    server.on("/json/state", HTTP_PUT, [](AsyncWebServerRequest *request) {
        AsyncResponseStream *response = request->beginResponseStream(mime_application_json);
        JsonDocument doc;
        dummyApi(doc, true);
        serializeJson(doc, *response);
        request->send(response);
    });

    server.on("/json", HTTP_GET, [](AsyncWebServerRequest *request) {
        AsyncResponseStream *response = request->beginResponseStream(mime_application_json);
        JsonDocument doc;
        dummyApi(doc, false);
        serializeJson(doc, *response);
        request->send(response);
    });

    // Scan WiFi
    server.on("/api/wifi_scan", HTTP_GET, [](AsyncWebServerRequest *request) {
        int n = WiFi.scanComplete();
        Serial.printf("[WiFi Scan] Status: %d\n", n);

        if (n == -1) { // scan in progress
            Serial.println("[WiFi Scan] Still scanning...");
            request->send(202, mime_application_json, "{\"status\":\"scanning\"}");
        } 
        else if (n == -2 || (n == 0)) { // restart scan
            Serial.println("[WiFi Scan] Starting new async scan...");
            WiFi.scanNetworks(true); 
            request->send(202, mime_application_json, "{\"status\":\"started\"}");
        } 
        else if (n > 0) { // scan is completed
            Serial.printf("[WiFi Scan] Found %d networks. Sending to client.\n", n);
            
            AsyncResponseStream *response = request->beginResponseStream(mime_application_json);
            JsonDocument doc;
            JsonArray array = doc.to<JsonArray>();
            
            for (int i = 0; i < n; ++i) {
                String ssid = WiFi.SSID(i);
                if (ssid.length() > 0) {
                    JsonObject net = array.add<JsonObject>();
                    net["ssid"] = ssid;
                    net["rssi"] = WiFi.RSSI(i);
                }
            }
            
            serializeJson(doc, *response);
            request->send(response);
            
            WiFi.scanDelete(); 
        }
        else {
            request->send(202, mime_application_json, "{\"status\":\"no_networks\"}");
        }
    });

    // Save WiFi
    server.on("/save_wifi", HTTP_POST, [](AsyncWebServerRequest *request) {
        if (request->hasParam("ssid", true) && request->hasParam("pass", true)) {
            cfg.wifi.ssid = request->getParam("ssid", true)->value();
            cfg.wifi.ssid.trim();
            if (cfg.wifi.ssid == "CUSTOM") {
                if (request->hasParam("ssid_custom", true)) {
                    cfg.wifi.ssid = request->getParam("ssid_custom", true)->value();
                    cfg.wifi.ssid.trim();
                } else {
                    cfg.wifi.ssid = "";
                }
            }
            cfg.wifi.password = request->getParam("pass", true)->value();
            saveConfig();
            
            auto newAddress = sanitizeMdnsService(cfg.deviceName);
            AsyncWebServerResponse *response = request->beginResponse(200, mime_text_html, 
                "<html><head><meta name='viewport' content='width=device-width, initial-scale=1'></head>"
                "<body style='background:#111;color:#fff;font-family:sans-serif;text-align:center;padding:10vh 15px;line-height:1.6;font-size:1.1rem;'>"
                "<h2>WiFi Saved</h2>"
                "<p>Switch your phone to your:</p><p style='font-weight:bold;'>" + cfg.wifi.ssid + "</p>"
                "<p>network and use address:</p>"
                "<p><a href='http://" + newAddress + ".local' style='color:#43a047;font-weight:bold;'>" + newAddress + ".local</a></p>"
                "<p style='font-size:0.9rem; color:#888;'>Alternatively check router for new IP.</p>"
                "<p style='color:#666;margin-top:40px;font-size:0.85rem;'>"
                "If connection fails, device will <b>switch back to WiFi AP</b> in 12s.</p>"
                "</body></html>");

            response->addHeader(F("Connection"), F("close"));
            request->send(response);
            Serial.println("WiFi Config saved. Rebooting in 2s...");
            managerScheduleReboot(2000);
        }
    });

    // Save settings
    server.on("/save_config", HTTP_POST, [](AsyncWebServerRequest *request) {
        bool needsRestart = false;

        if (request->hasParam("type", true)) {
            uint8_t t = request->getParam("type", true)->value().toInt();
            if (t != (uint8_t)cfg.led.type)
            {
                #ifdef USE_FASTLED
                    needsRestart = true;
                #endif

                cfg.led.type = (LedType)t;
            }
        }
        if (request->hasParam("dataPin", true)) {
            uint8_t p = request->getParam("dataPin", true)->value().toInt();
            if (p != cfg.led.dataPin)
            {
                #ifdef USE_FASTLED
                    needsRestart = true;
                #endif

                cfg.led.dataPin = p;
            }
        }
        if (request->hasParam("clockPin", true)) {
            uint8_t p = request->getParam("clockPin", true)->value().toInt();
            if (p != cfg.led.clockPin)
            {
                #ifdef USE_FASTLED
                    needsRestart = true;
                #endif

                cfg.led.clockPin = p;
            }
        }
        if (request->hasParam("numLeds", true)) {
            uint16_t n = request->getParam("numLeds", true)->value().toInt();
            if (n != cfg.led.numLeds && n <= MAX_LEDS)
            {
                #ifdef USE_FASTLED
                    needsRestart = true;
                #endif
                                
                cfg.led.numLeds = n;
            }
        }

        // live updates
        if (request->hasParam("brightness", true)) cfg.led.brightness = constrain(request->getParam("brightness", true)->value().toInt(), 1, 255);
        if (request->hasParam("r", true)) cfg.led.r = constrain(request->getParam("r", true)->value().toInt(), 0, 255);
        if (request->hasParam("g", true)) cfg.led.g = constrain(request->getParam("g", true)->value().toInt(), 0, 255);
        if (request->hasParam("b", true)) cfg.led.b = constrain(request->getParam("b", true)->value().toInt(), 0, 255);
        if (request->hasParam("effect", true)) cfg.led.effect = request->getParam("effect", true)->value().toInt();

        if (request->hasParam("calGain", true)) cfg.led.calibration.gain = constrain(request->getParam("calGain", true)->value().toInt(), 0, 255);
        if (request->hasParam("calRed", true)) cfg.led.calibration.red = constrain(request->getParam("calRed", true)->value().toInt(), 0, 255);
        if (request->hasParam("calGreen", true)) cfg.led.calibration.green = constrain(request->getParam("calGreen", true)->value().toInt(), 0, 255);
        if (request->hasParam("calBlue", true)) cfg.led.calibration.blue = constrain(request->getParam("calBlue", true)->value().toInt(), 0, 255);

        if (request->hasParam("extraMdnsTag", true)) {            
            auto val = sanitizeMdnsService(request->getParam("extraMdnsTag", true)->value());
            needsRestart = (cfg.extraMdnsTag != val);
            cfg.extraMdnsTag = val;
        }

        saveConfig();

        if (needsRestart) {
            request->send(200, mime_application_json, "{\"status\":\"reboot\"}");
            Serial.println("Configuration saved. Rebooting in 2s...");
            managerScheduleReboot(2000);
        }
        else {
            managerScheduleApplyConfig();
            request->send(200, mime_application_json, "{\"status\":\"ok\"}");
        }
    });

    // Current config
    server.on("/api/get_current_config", HTTP_GET, [](AsyncWebServerRequest *request) {        
        AsyncResponseStream *response = request->beginResponseStream(mime_application_json);                
        JsonDocument doc;         
        JsonObject led = doc["config"].to<JsonObject>();
        led["type"]         = (int)cfg.led.type;
        led["dataPin"]      = cfg.led.dataPin;
        led["clockPin"]     = cfg.led.clockPin;
        led["numLeds"]      = cfg.led.numLeds;
        led["brightness"]   = cfg.led.brightness;
        led["r"]            = cfg.led.r;
        led["g"]            = cfg.led.g;
        led["b"]            = cfg.led.b;
        led["effect"]       = cfg.led.effect;
        led["extraMdnsTag"] = cfg.extraMdnsTag;

        led["calGain"]  = cfg.led.calibration.gain;
        led["calRed"]   = cfg.led.calibration.red;
        led["calGreen"] = cfg.led.calibration.green;
        led["calBlue"]  = cfg.led.calibration.blue;

        serializeJson(doc, *response);
        request->send(response);
    });

    // Internal stats API
    server.on("/api/stats", HTTP_GET, [](AsyncWebServerRequest *request) {
        AsyncResponseStream *response = request->beginResponseStream(mime_application_json);
        JsonDocument doc;
        
        doc["DEVICE"] = cfg.deviceName;
        doc["IP"]     = WiFi.localIP().toString();
        doc["RSSI"]   = String(WiFi.RSSI()) + " dBm";
        doc["UPTIME"] = String(millis() / 1000);
        doc["HEAP"]   = String(getFreeHeap() / 1024) + " kB";
        doc["ARCH"]   = getDeviceArch();
        doc["FPS"]    = stats.lastIntervalRenderedFrames;
        doc["SKIPPED"]    = stats.lastIntervalSkippedFrames;

        serializeJson(doc, *response);
        request->send(response);
    });

    // Not found and captivity mode
    server.onNotFound([](AsyncWebServerRequest *request) {
        if (isAPMode() && request->method() == HTTP_GET) {
            request->redirect("/");
        }
        else {
            request->send(404, mime_text_plain, "Not found");
        }
    });
    server.on("/fwlink", HTTP_GET, [](AsyncWebServerRequest *request){ request->redirect("/"); });
    server.on("/check_generate_204", HTTP_GET, [](AsyncWebServerRequest *request){ request->redirect("/"); });
    server.on("/generate_204", HTTP_ANY, [](AsyncWebServerRequest *request){ request->redirect("/"); });

    setupStaticHandlers(server);
}

void sendEmbeddedFile(AsyncWebServerRequest *request, const uint8_t* content, uint32_t len, const char* contentType)
{
    AsyncWebServerResponse *response = request->beginResponse(200, contentType, content, len);

    if (response != nullptr) 
    {
        response->addHeader(F("Content-Encoding"), F("gzip"));
        response->addHeader(F("Connection"), F("close"));
        if (strcmp(contentType, "text/css") == 0) 
        {
            response->addHeader(F("Cache-Control"), F("public, max-age=31536000"));
        }
        request->send(response);
    }
}

// resources are generated by web_embedder.py
void setupStaticHandlers(AsyncWebServer& server)
{
    for (uint16_t i = 0; i < webResourcesCount; i++)
    {
        const char* currentUrl;
        #if defined(ESP8266)
            currentUrl = (const char*)pgm_read_ptr(&(webResources[i].url));
        #else
            currentUrl = webResources[i].url;
        #endif

        server.on(currentUrl, HTTP_GET, [i](AsyncWebServerRequest *request)
        {
            WebResource res;
            #if defined(ESP8266)
                memcpy_P(&res, &webResources[i], sizeof(WebResource));
            #else
                res = webResources[i];
            #endif
            sendEmbeddedFile(request, res.data, res.len, res.mime);
        });
    }
}