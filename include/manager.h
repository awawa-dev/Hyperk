// File: include/manager.h

#pragma once

struct Stats{
    volatile uint16_t renderedFrames = 0;
    volatile uint16_t skippedFrames = 0;
    uint32_t currentTag = 0;
    uint16_t lastIntervalRenderedFrames = 0;
    uint16_t lastIntervalSkippedFrames = 0;
};

extern Stats stats;

void managerScheduleApplyConfig();
void managerScheduleReboot(uint32_t delay_ms);
void managerLoop();