/* ota.cpp
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
#if defined(ESP32)
  #include <Update.h>
#elif defined(ESP8266)
  #include <Updater.h>
#elif defined(ARDUINO_ARCH_RP2040)
  #include <Updater.h>
#endif
#include "ota.h"
#include "logger.h"
#include "manager.h"
#include "utils.h"

namespace {
    bool customValidationFailed = false;
}

void otaUpdateHandler(AsyncWebServer &server) {
    server.on("/ota", HTTP_POST, [](AsyncWebServerRequest *request) {
        bool error = Update.hasError() || customValidationFailed;
        AsyncWebServerResponse *response = request->beginResponse(error ? 500 : 200, "text/plain", error ? "FAIL" : "OK");
        response->addHeader("Connection", "close");
        request->send(response);
        
        if (!error) {
            Log::debug("OTA Update successful. Rebooting...");
            managerScheduleReboot(1000);
        }
    }, [](AsyncWebServerRequest *request, String filename, size_t index, uint8_t *data, size_t len, bool final) {
        if (!index) {
            Log::debug("OTA Update Start: ", filename);

            customValidationFailed = false;

            if (request->hasHeader("hyperk-ota-firmware-name")) {
                String firmwareName = request->getHeader("hyperk-ota-firmware-name")->value();
                firmwareName.toLowerCase();
                String expectedPlatform = String(PIO_ENV_NAME) + ".";
                Log::debug("OTA firmware name: ", firmwareName, ", searching for: ", expectedPlatform, " tag");
                if (firmwareName.indexOf(expectedPlatform) < 0)  {
                    #ifdef ARDUINO_ARCH_ESP32
                        Update.abort();
                    #endif
                    customValidationFailed = true;
                    Log::debug("OTA incorrect firmware version");
                    return;
                 }
            } else {
                #ifdef ARDUINO_ARCH_ESP32
                    Update.abort();
                #endif
                customValidationFailed = true;
                Log::debug("OTA missing hyperk-ota-firmware-name header");
                return;
            }

#if defined(ESP8266)
            Update.runAsync(true);
            size_t maxSpace = (ESP.getFreeSketchSpace() - 0x1000) & 0xFFFFF000;
#elif defined(ARDUINO_ARCH_RP2040)    
            size_t maxSpace = 0;
#elif defined(ARDUINO_ARCH_ESP32)
            size_t maxSpace = UPDATE_SIZE_UNKNOWN;
#endif
            if (request->hasHeader("hyperk-ota-firmware-size")) {
                maxSpace = request->getHeader("hyperk-ota-firmware-size")->value().toInt();
                Log::debug("OTA expected firmware size: ", maxSpace);
            } else {
                #ifdef ARDUINO_ARCH_ESP32
                    Update.abort();
                #endif
                customValidationFailed = true;
                Log::debug("OTA missing hyperk-ota-firmware-size header");
                return;
            }

            if (!Update.begin(maxSpace, U_FLASH)) {
                Update.printError(Serial);
            }
        }

        if (!Update.hasError() && !customValidationFailed) {
            if (Update.write(data, len) != len) {
                Update.printError(Serial);
            }
        }

        if (final) {
            if (Update.end()) {
                Log::debug("OTA Update Success: ", index + len, " bytes");
            } else {
                Update.printError(Serial);
            }
        }
    });
}