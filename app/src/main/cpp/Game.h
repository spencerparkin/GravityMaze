#pragma once

#include <EGL/egl.h>
#include <memory>
#include "Engine.h"

struct android_app;

class Game
{
public:
    Game(android_app* app);
    virtual ~Game();

    bool Init();
    bool Shutdown();
    void HandleInput();
    void Render();

private:

    android_app* app;
    bool initialized;
    EGLDisplay display;
    EGLSurface surface;
    EGLContext context;
    EGLint surfaceWidth;
    EGLint surfaceHeight;
    PlanarPhysics::Engine physicsEngine;
};