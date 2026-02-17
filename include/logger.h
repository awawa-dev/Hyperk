// File: include/logger.h
#pragma once
#include <Arduino.h>

namespace Log {
    template<typename... Args>
    void inline debug(Args... args) {
        #ifdef DEBUG_LOG
            (Serial.print(args), ...);
            Serial.println();
        #endif
    }
    void inline raw(const char* message, char* buffer, int len) {
        #ifdef DEBUG_LOG
            Serial.print(message);
            Serial.write(buffer,len);
            Serial.println();
        #endif
    } 
}