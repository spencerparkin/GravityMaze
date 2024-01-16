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

    // TODO: Finally note here and I'm just going to stop developing this program for a
    //       while, because Android sucks.  One of the big problems I have here is that
    //       I'm supposedly trying to do everything on the main UI thread (or so I've
    //       been told) and that's what's causing my app to render/run slower than it should.
    //       So some big refactoring needs to happen where all the game/physics/animation
    //       logic is in its own thread and then all the rendering is in another thread.
    //       But as far as I can tell, the rendering thread can't be its own thread; it
    //       has to be the main thread or else the eglSwapBuffers() call just won't work.
    //       Honestly, I'm sick of this, and I just don't care anymore.  I'm spending too
    //       much time away from family and work (time I just don't have) on this stupid
    //       program that is just a complete waste of time.

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