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

    clock_t timeA = 0;
    std::list<double> frameTimeFifo;
    long frameCount = 0;

    do
    {
        frameCount++;
        clock_t timeB = ::clock();
        if(timeA != 0L)
        {
            double elapsedTimeSeconds = double(timeB - timeA) / double(CLOCKS_PER_SEC);
            frameTimeFifo.push_back(elapsedTimeSeconds);
            while(frameTimeFifo.size() > 100)
                frameTimeFifo.erase(frameTimeFifo.begin());
        }
        timeA = timeB;

        if(frameCount % 50 == 0)
        {
            double minTimeSeconds = std::numeric_limits<double>::max();
            double maxTimeSeconds = std::numeric_limits<double>::min();
            double totalTimeSeconds = 0.0;
            for(double elapsedTimeSeconds : frameTimeFifo)
            {
                totalTimeSeconds += elapsedTimeSeconds;
                if(minTimeSeconds > elapsedTimeSeconds)
                    minTimeSeconds = elapsedTimeSeconds;
                if(maxTimeSeconds < elapsedTimeSeconds)
                    maxTimeSeconds = elapsedTimeSeconds;
            }
            double averageFPS = double(frameTimeFifo.size()) / totalTimeSeconds;
            double minFPS = 1.0 / maxTimeSeconds;
            double maxFPS = 1.0 / minTimeSeconds;
            aout << "=======================================" << std::endl;
            aout << "Average FPS: " << averageFPS << std::endl;
            aout << "    Min FPS: " << minFPS << std::endl;
            aout << "    Max FPS: " << maxFPS << std::endl;
        }

        Game* game = nullptr;

        if (app->userData)
            game = reinterpret_cast<Game*>(app->userData);

        void* data = nullptr;
        int events = 0;
        int id = ALooper_pollOnce(0, nullptr, &events, &data);
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