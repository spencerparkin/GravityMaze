#pragma once

#include <EGL/egl.h>
#include <memory>
#include <android/sensor.h>
#include "Engine.h"
#include "Maze.h"
#include "DrawHelper.h"
#include "Options.h"

struct android_app;

class Game
{
public:
    Game(android_app* app);
    virtual ~Game();

    bool Init();
    bool Shutdown();
    void Render();
    void GenerateNextMaze();
    void Tick();
    void HandleSensorEvent(void* data);

    enum
    {
        SENSOR_EVENT_ID = 100
    };

    const Options& GetOptions() const { return this->options; }

private:

    android_app* app;
    bool initialized;
    EGLDisplay display;
    EGLSurface surface;
    EGLContext context;
    EGLint surfaceWidth;
    EGLint surfaceHeight;
    PlanarPhysics::Engine physicsEngine;
    Maze maze;
    DrawHelper drawHelper;
    ASensorManager* sensorManager;
    const ASensor* gravitySensor;
    ASensorEventQueue* sensorEventQueue;
    Options options;
};