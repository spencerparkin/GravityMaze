#include <jni.h>
#include "AndroidOut.h"
#include "Game.h"
#include <game-activity/GameActivity.cpp>
#include <game-text-input/gametextinput.cpp>

extern "C" {

#include <game-activity/native_app_glue/android_native_app_glue.c>

void HandleCommand(android_app* app, int32_t cmd)
{
    switch (cmd)
    {
        case APP_CMD_INIT_WINDOW:
        {
            Game* game = new Game(app);

            if(!game->Init())
            {
                // TODO: Handle error here somehow?
            }

            game->GenerateNextMaze();

            app->userData = game;
            break;
        }
        case APP_CMD_TERM_WINDOW:
        {
            if (app->userData)
            {
                auto game = reinterpret_cast<Game*>(app->userData);
                if(!game->Shutdown())
                {
                    // TODO: Handle error here somehow?
                }

                delete game;
                app->userData = nullptr;
            }
            break;
        }
        default:
        {
            break;
        }
    }
}

bool MotienEventFilter(const GameActivityMotionEvent *motionEvent)
{
    auto sourceClass = motionEvent->source & AINPUT_SOURCE_CLASS_MASK;
    return sourceClass == AINPUT_SOURCE_SENSOR;
}

void android_main(struct android_app* app)
{
    aout << "Android main begin!" << std::endl;

    app->onAppCmd = HandleCommand;

    android_app_set_motion_event_filter(app, MotienEventFilter);

    int events = 0;
    android_poll_source* source = nullptr;
    do
    {
        if (ALooper_pollAll(0, nullptr, &events, (void **)&source) >= 0)
        {
            if (source)
                source->process(app, source);
        }

        if (app->userData)
        {
            auto *game = reinterpret_cast<Game*>(app->userData);
            game->HandleInput();
            game->Tick();
            game->Render();
        }
    }
    while (!app->destroyRequested);

    aout << "Android main finished!" << std::endl;
}

} // extern "C" {