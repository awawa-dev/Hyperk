#pragma once
#include "effects.h"
#include <array>

/**
* @brief Always use EFFECT_NAMESPACE for your custom effect, the effect's name is it's folder name infact
*/
namespace EFFECT_NAMESPACE {
    /**
     * @brief Performs one cycle of the effect.
     * @return false if the effect is finished, true otherwise.
     */
    inline bool run(const EffectConfig& effectConfig, EffectSetup &effectSetup, EffectRun& effectRun) {
        // If it's the first call, just do the setup and skip the rest
        if (!effectRun.deltaMicroseconds) {
            effectSetup.runIntervalMicroseconds = 250000; // 4Hz, every 250000 microseconds = 0.25 second
            effectSetup.useGamma = false;
            return true;
        }

        // process the effect
        for(int i = 0; i < effectConfig.numLeds; i++){
            effectRun.setLedColorRgb(i, effectRun.currentStaticColor.red, effectRun.currentStaticColor.green, effectRun.currentStaticColor.blue);
        }
        return true;
    }
};
