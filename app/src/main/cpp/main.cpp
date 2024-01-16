#include <jni.h>
#include "AndroidOut.h"
#include "GameRender.h"
#include "GameLogic.h"
#include <game-activity/GameActivity.cpp>
#include <game-text-input/gametextinput.cpp>

extern "C" {

#include <game-activity/native_app_glue/android_native_app_glue.c>

void android_main(struct android_app* app)
{
    aout << "Android main begin!" << std::endl;

    GameRender* gameRender = new GameRender(app);
    gameRender->Setup();

    GameLogic* gameLogic = new GameLogic(gameRender);
    gameLogic->Setup();

    while(gameRender->Tick())
    {
    }

    gameLogic->Shutdown();
    delete gameLogic;
    gameLogic = nullptr;

    gameRender->Shutdown();
    delete gameRender;
    gameRender = nullptr;

    aout << "Android main finished!" << std::endl;
}

} // extern "C" {