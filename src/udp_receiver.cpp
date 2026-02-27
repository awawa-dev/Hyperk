/* udp_receiver.cpp
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
#include <WiFiUdp.h>
#include "udp_receiver.h"
#include "config.h"
#include "leds.h"
#include "volatile_state.h"

namespace {
    struct DDPHeader {
        uint8_t  flags;
        uint8_t  reserved;
        uint8_t  type;
        uint8_t  channel;
        uint32_t offset;
        uint16_t len;
    } __attribute__((packed));

    constexpr auto MTU_SIZE = 1500;
}

void clearBufferUDP(WiFiUDP &udp, int howMuch) {
    while (howMuch-- > 0 ) {
        udp.read();
    }
}

void handleDDP(WiFiUDP& udp) {
    int packetSize = udp.parsePacket();
    if (packetSize < 5 || packetSize > MTU_SIZE)
    {
        clearBufferUDP(udp, packetSize);
        return;
    }

    #ifdef USE_FASTLED
        const bool brightnessControl = false;
        auto setPixel = Leds::setLed<false>;
        auto setPixelW = Leds::setLedW<false>;
    #else
        const bool brightnessControl = (Volatile::state.brightness != 255);
        auto setPixel = brightnessControl ? Leds::setLed<true> : Leds::setLed<false>;
        auto setPixelW = brightnessControl ? Leds::setLedW<true> : Leds::setLedW<false>;
    #endif     

    uint8_t buffer[packetSize];    
    uint8_t* endBuffer = &(buffer[0]) + udp.read(buffer, packetSize);        
    DDPHeader* hdr = reinterpret_cast<DDPHeader*>(&(buffer[0]));
    auto rgb = &(buffer[0]) + sizeof(DDPHeader);

    if ((hdr->flags & 0xc0) != 0x40) return;

    if (hdr->flags & 0x02) { 
        hdr->flags = 0x40 | 0x04;
        hdr->type  = 0x10; 
        
        char info[128];
        int infoLen = snprintf(info, sizeof(info),
                               "1;%d;%s;%s;%s;%d;4048", 
                               1, APP_NAME, "PicoESP", APP_VERSION, Leds::getLedsNumber());

        hdr->len = __builtin_bswap16(infoLen);

        udp.beginPacket(udp.remoteIP(), udp.remotePort());
        udp.write((uint8_t*)hdr, sizeof(DDPHeader));
        udp.write((uint8_t*)info, infoLen);
        udp.endPacket();
        return;
    }

    uint32_t channelOffset = __builtin_bswap32(hdr->offset);
    uint16_t dataLen       = __builtin_bswap16(hdr->len);
    int expectedSize = sizeof(DDPHeader) + dataLen + ((hdr->flags & 0x10) ? 4 : 0);

    if (packetSize < expectedSize)
        return;

    if (hdr->flags & 0x10) {
        rgb += 4;
    }

    const int bytesPerLed = ((hdr->type & 0x38) == 0x18 || hdr->type == 0x03) ? 4 : 3;
    const int offset = channelOffset / bytesPerLed;
    const int maxLedNumber = Leds::getLedsNumber();

    if (bytesPerLed == 4)
    {
        for (int i = offset; rgb + 3 < endBuffer && i < maxLedNumber; rgb += 4)
            setPixelW(i++, rgb[0], rgb[1], rgb[2], rgb[3]);        
    }
    else
    {
        for (int i = offset; rgb + 2 < endBuffer && i < maxLedNumber; rgb += 3)
            setPixel(i++, rgb[0], rgb[1], rgb[2]);
    }

    if (hdr->flags & 0x01) {
        Volatile::updateStreamTimeout();
        Leds::renderLed(true);
    }
}

void handleRealTime(WiFiUDP& udp) {    
    int packetSize = udp.parsePacket();
    if (packetSize < 5 || packetSize >= MTU_SIZE)
    {
        clearBufferUDP(udp, packetSize);
        return;
    }

    #ifdef USE_FASTLED
        const bool brightnessControl = false;
        auto setPixel = Leds::setLed<false>;
        auto setPixelW = Leds::setLedW<false>;
    #else
        const bool brightnessControl = (Volatile::state.brightness != 255);
        auto setPixel = brightnessControl ? Leds::setLed<true> : Leds::setLed<false>;
        auto setPixelW = brightnessControl ? Leds::setLedW<true> : Leds::setLedW<false>;
    #endif     

    uint8_t buffer[packetSize]; 
    uint8_t* endBuffer = &(buffer[0]) + udp.read(buffer, packetSize);    

    const uint8_t protocolType = buffer[0];
    const int maxLedNumber = Leds::getLedsNumber();
    const uint8_t* rgb = nullptr;

    // 0x01 = WARLS, 0x02 = DNRGB, 0x04 = DRGB (indexed), 0x03 = DRGBW support (RGBW)    
    if (protocolType == 0x02) {
        rgb = &(buffer[2]);
        for (int i = 0; rgb + 2 < endBuffer && i < maxLedNumber; rgb += 3) {
            setPixel(i++, rgb[0], rgb[1], rgb[2]);
        }
    }
    else if (protocolType == 0x04) {        
        uint16_t offset = 0;

        // offset detection: pocket size including 2 header + optional 2 offset % 3 == 1, guuarded by packetSize >= 4
        if (packetSize % 3 == 1)
        { 
            offset = (buffer[2] << 8) | buffer[3];
            rgb = &(buffer[4]);
        }
        else
        {
            rgb = &(buffer[2]);
        }

        for (int i = offset; rgb + 2 < endBuffer && i < maxLedNumber; rgb += 3) {
            setPixel(i++, rgb[0], rgb[1], rgb[2]);
        }
    }
    else if (protocolType == 0x03) { // DRGBW support (r, g, b, w...)
        rgb = &(buffer[2]);
        for (int i = 0; rgb + 3 < endBuffer && i < maxLedNumber; rgb += 4) {
            setPixelW(i++, rgb[0], rgb[1], rgb[2], rgb[3]);
        }
    }
    else
        return;

    Volatile::updateStreamTimeout();
    Leds::renderLed(true);
}

void handleRAW(WiFiUDP& udp)
{
    int packetSize = udp.parsePacket();
    if (packetSize < 3 || packetSize >= MTU_SIZE)
    {
        clearBufferUDP(udp, packetSize);
        return;
    }

    #ifdef USE_FASTLED
        const bool brightnessControl = false;
        auto setPixel = Leds::setLed<false>;
        auto setPixelW = Leds::setLedW<false>;
    #else
        const bool brightnessControl = (Volatile::state.brightness != 255);
        auto setPixel = brightnessControl ? Leds::setLed<true> : Leds::setLed<false>;
        auto setPixelW = brightnessControl ? Leds::setLedW<true> : Leds::setLedW<false>;
    #endif    

    uint8_t buffer[packetSize]; 
    uint8_t* endBuffer = &(buffer[0]) + udp.read(buffer, packetSize);    

    const int maxLedNumber = Leds::getLedsNumber();
    const uint8_t* rgb = &(buffer[0]);

    for (int i = 0; rgb + 2 < endBuffer && i < maxLedNumber; rgb += 3) {
        setPixel(i++, rgb[0], rgb[1], rgb[2]);
    }

    Volatile::updateStreamTimeout();
    Leds::renderLed(true);
}
