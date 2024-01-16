#pragma once

#include <game-activity/native_app_glue/android_native_app_glue.h>
#include <EGL/egl.h>
#include <memory>
#include <android/sensor.h>
#include "AudioSubSystem.h"
#include "MidiManager.h"
#include "Engine.h"
#include "Maze.h"
#include "DrawHelper.h"
#include "Options.h"
#include "TimeKeeper.h"
#include "Math/GeometricAlgebra/Vector2D.h"

struct android_app;

// We don't just render here; we also handle sensor input and audio output.
class GameRender
{
public:
    GameRender(android_app* app);
    virtual ~GameRender();

    bool Setup();
    bool Shutdown();
    bool SetupWindow();
    bool ShutdownWindow();
    bool Tick();
    void HandleSensorEvent(void* data);
    void HandleTapEvents();

    enum
    {
        SENSOR_EVENT_ID = 100
    };

    Options& GetOptions() { return this->options; }
    android_app* GetApp() { return this->app; }

    static void HandleAndroidCommand(android_app* app, int32_t cmd);
    static bool MotionEventFilter(const GameActivityMotionEvent* motionEvent);

    bool CanRender();

    DrawHelper* GetDrawHelper() { return &this->drawHelper; }
    double GetAspectRatio() const;

    const PlanarPhysics::Vector2D& GetGravityVector() const { return this->gravityVector; }

private:

    android_app* app;
    bool initialized;
    EGLDisplay display;
    EGLSurface surface;
    EGLContext context;
    EGLint surfaceWidth;
    EGLint surfaceHeight;
    DrawHelper drawHelper;
    ASensorManager* sensorManager;
    const ASensor* gravitySensor;
    ASensorEventQueue* sensorEventQueue;
    Options options;
    AudioSubSystem audioSubSystem;
    MidiManager midiManager;
    TimeKeeper timeKeeper;
    PlanarPhysics::Vector2D gravityVector;
};