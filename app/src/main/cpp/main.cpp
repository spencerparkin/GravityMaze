#include <jni.h>
#include "AndroidOut.h"
#include "Game.h"
#include <game-activity/GameActivity.cpp>
#include <game-text-input/gametextinput.cpp>

extern "C" {

#include <game-activity/native_app_glue/android_native_app_glue.c>

void android_main(struct android_app* app)
{
    aout << "Android main begin!" << std::endl;

    Game* game = new Game(app);
    game->Setup();

    while(game->Tick())
    {
    }

    game->Shutdown();
    delete game;
    game = nullptr;

    aout << "Android main finished!" << std::endl;
}

} // extern "C" {