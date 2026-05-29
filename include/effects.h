#pragma once
#include <ArduinoJson.h>
#include <stdint.h>
#include <array>
#include <memory>
#include "etl/vector.h"

struct EffectConfig{
    /**
     * @brief RGB or RGBW led strip.
     */     
    bool isRgbw;

    /**
     * @brief Total number of LEDs.
     */     
    uint16_t numLeds;    

    /**
     * @brief Last segment contains numLeds value. Useful for segment [begin_current, begin_next) segment iteration.
     */    
    etl::vector<int16_t, 9> segmentStarts;
};

struct EffectSetup {
    /**
     * @brief Use gamma 2.2 or not for the effect. Take into the account during next invoke.
     */     
    bool useGamma;

    /**
     * @brief Delay before next invoke in microseconds. 1s = 1000 * 1000 microseconds.
     */     
    unsigned long runIntervalMicroseconds;

    using UserDataDeleterFn = void(*)(void*);
    /**
     * @brief General purpose user data container (POD). Can store information between Effect::run invokes.
     */    
    std::unique_ptr<void, UserDataDeleterFn> userData{nullptr, [](void*){}};

    /**
     * @brief Assign general purpose user data container (POD). The container is automaticly deleted alongside the effect.
     */      
    template <typename T>
    void assign(T* rawPointerToUserData) {
        static_assert(std::is_standard_layout<T>::value, "ERROR: user data structure MUST be a POD (Standard Layout) type!");
        static_assert(std::is_trivially_destructible<T>::value, "ERROR: user data structure cannot have complex destructors (e.g., no std::string inside)!");        
        userData = std::unique_ptr<void, UserDataDeleterFn>(rawPointerToUserData, [](void* p) { delete static_cast<T*>(p); });
    }    

    /**
     * @brief Optional effect starting params.
     */
    std::unique_ptr<JsonDocument> optionalEffectStartingParams;    
};

struct EffectRun {
    /**
     * @brief Time (microseconds) since the last time called or 0 when first time invoked, 1s = 1000 * 1000 microseconds.
     */       
    unsigned long deltaMicroseconds;

    using setLedColorRgbFunc = void(*)(uint16_t, uint8_t, uint8_t, uint8_t);
    /**
     * @brief Use this to set LEDs to selected color (index, red, greeb, blue). On RGBW LED strips the white channel is calculated using calibration settings.
     */     
    setLedColorRgbFunc setLedColorRgb;
    using setLedColorRgbWFunc = void(*)(uint16_t, uint8_t, uint8_t, uint8_t, uint8_t);
    /**
     * @brief Use this to set LEDs to selected color (index, red, greeb, blue, white)
     */     
    setLedColorRgbWFunc setLedColorRgbW;
    using getLedColorRgbWFunc = void(*)(uint16_t, uint8_t&, uint8_t&, uint8_t&, uint8_t&);
    /**
     * @brief Use this to get selected LEDs color (index, red, greeb, blue, white)
     */     
    getLedColorRgbWFunc getLedColorRgbW;

    /**
     * @brief Current static color.
     */     
    struct {
        uint8_t red, green, blue;
    } currentStaticColor;    
};

////////////////////////////////////////////////////////////////////////////////////////////

namespace EffectUtils{
    void fast_hsv2rgb(uint8_t h, uint8_t s, uint8_t v, uint8_t *r, uint8_t *g, uint8_t *b);
    void fast_rgb2hsv(uint8_t r, uint8_t g, uint8_t b, uint8_t *h, uint8_t *s, uint8_t *v);
};

