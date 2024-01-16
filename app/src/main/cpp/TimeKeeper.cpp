#include "TimeKeeper.h"

TimeKeeper::TimeKeeper()
{
    this->lastTime = 0;
    this->elapsedTimeSeconds = 0.0;
    this->frameRateFPS = 0.0;
}

/*virtual*/ TimeKeeper::~TimeKeeper()
{
}

void TimeKeeper::Tick()
{
    clock_t currentTime = ::clock();

    this->elapsedTimeSeconds = 0.0;
    this->frameRateFPS = 0.0;

    if(this->lastTime != 0)
    {
        // I'm not sure how accurate this is on Android.  It seems wrong.
        this->elapsedTimeSeconds = double(currentTime - this->lastTime) / double(CLOCKS_PER_SEC);
        this->frameRateFPS = 1.0 / elapsedTimeSeconds;
    }

    this->lastTime = currentTime;
}