#pragma once

#include <pthread.h>

class GameLogic
{
public:
    GameLogic();
    virtual ~GameLogic();

    bool Setup();
    bool Shutdown();
    bool Tick();

private:
    static void* ThreadEntryPoint(void* arg);
    void ThreadFunc();

    bool keepTicking;
    pthread_t threadHandle;
};