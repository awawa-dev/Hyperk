// File: include/leds.h
#pragma once
#include <Arduino.h>
#include <vector>
#include "config.h"

void applyLedConfig();
int getLedsNumber();
void setLed(int index, uint8_t r, uint8_t g, uint8_t b);
void setLed(int index, uint8_t r, uint8_t g, uint8_t b, uint8_t w);
void checkDeleyedRender();
void renderLed(bool isNewFrame);
