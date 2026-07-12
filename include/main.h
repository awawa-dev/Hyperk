// File: include/main.h

#pragma once

bool isAPMode();

namespace WebServerProvider{
    void setupWebServer();
};

namespace Mdns{
    void startMDNS();
};

namespace Log {
    #ifdef ENABLE_DEBUG    
        template<typename... Args>
        void inline SERIAL_LOG(Args... args) {
            (Serial.print(args), ...);
            Serial.println();
        }
        void inline raw(const char* message, char* buffer, int len) {
            Serial.print(message);
            Serial.write(buffer,len);
            Serial.println();
        }
    #else
        static inline void noop() {}
        #define SERIAL_LOG(...) noop()
    #endif
}

namespace Manager {
    void scheduleApplyConfig();
    void scheduleReboot(uint32_t delay_ms);
    void cancelScheduledReboot();
    void processEvents();
};

namespace UdpReceiver {
    void handleDDP(WiFiUDP& udp);
    void handleRealTime(WiFiUDP& udp);
    void handleRAW(WiFiUDP& udp);
};

namespace Config {
    bool loadConfig();
    unsigned int getSeriaPortSpeed();
    bool getWifiCredentials(const char*& ssid, const char*& pass);
};

namespace SerialPort {
    void init(unsigned int speed);
    uint32_t getFreeSerialPortStack();
    #if defined(SERIAL_PORT_LITE)
        void processEventsFromLoop();
    #endif
};

namespace Leds {
    void applyLedConfig();
};
