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

void android_main(struct android_app* app)
{
    aout << "Android main begin!" << std::endl;

    app->onAppCmd = HandleCommand;

    do
    {
        Game* game = nullptr;

        if (app->userData)
            game = reinterpret_cast<Game*>(app->userData);

        void* data = nullptr;
        int events = 0;
        int id = ALooper_pollAll(0, nullptr, &events, &data);
        if (id >= 0)
        {
            switch(id)
            {
                case Game::SENSOR_EVENT_ID:
                {
                    if(game)
                        game->HandleSensorEvent(data);
                    break;
                }
                default:
                {
                    auto source = reinterpret_cast<android_poll_source*>(data);
                    if (source)
                        source->process(app, source);
                    break;
                }
            }
        }

        if(game)
        {
            game->Tick();
            game->Render();
        }
    }
    while (!app->destroyRequested);

    aout << "Android main finished!" << std::endl;
}

} // extern "C" {