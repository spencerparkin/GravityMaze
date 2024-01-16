#pragma once

#include <time.h>

class TimeKeeper
{
public:
    TimeKeeper();
    virtual ~TimeKeeper();

    void Tick();

    double GetElapsedTimeSeconds() { return this->elapsedTimeSeconds; }
    double GetFrameRate() { return this->frameRateFPS; }

private:
    clock_t lastTime;
    double elapsedTimeSeconds;
    double frameRateFPS;
};