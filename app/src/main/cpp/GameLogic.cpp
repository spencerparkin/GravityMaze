#include "GameLogic.h"

GameLogic::GameLogic()
{
    this->keepTicking = true;
    this->threadHandle = 0;
}

/*virtual*/ GameLogic::~GameLogic()
{
}

bool GameLogic::Setup()
{
    this->keepTicking = true;

    if(0 != pthread_create(&this->threadHandle, nullptr, &GameLogic::ThreadEntryPoint, this))
        return false;

    return true;
}

bool GameLogic::Shutdown()
{
    this->keepTicking = false;

    if(this->threadHandle)
    {
        pthread_join(this->threadHandle, nullptr);
        this->threadHandle = 0;
    }

    return true;
}

bool GameLogic::Tick()
{
    return true;
}

/*static*/ void* GameLogic::ThreadEntryPoint(void* arg)
{
    auto gameLogic = static_cast<GameLogic*>(arg);
    gameLogic->ThreadFunc();
    return nullptr;
}

void GameLogic::ThreadFunc()
{
    while(this->keepTicking)
        if(!this->Tick())
            break;
}