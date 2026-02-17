// File: include/storage.h

#pragma once
#include "config.h"

namespace Storage {
    bool loadConfig(AppConfig& cfg);
    bool saveConfig(const AppConfig& cfg);
};
