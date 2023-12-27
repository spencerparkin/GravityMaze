#pragma once

#include <EGL/egl.h>
#include <memory>
#include <android/sensor.h>
#include "Engine.h"
#include "Maze.h"
#include "DrawHelper.h"
#include "Options.h"
#include "Progress.h"

struct android_app;

class Game
{
public:
    Game(android_app* app);
    virtual ~Game();

    bool Init();
    bool Shutdown();
    void Render();
    void Tick();
    void HandleSensorEvent(void* data);

    enum
    {
        SENSOR_EVENT_ID = 100
    };

    const Options& GetOptions() const { return this->options; }

    class State
    {
    public:
        State(Game* game);
        virtual ~State();

        virtual void Enter();
        virtual void Leave();
        virtual State* Tick(double deltaTime);
        virtual double GetTransitionAlpha() const;

        Game* game;
    };

    class GenerateMazeState : public State
    {
    public:
        GenerateMazeState(Game* game);
        virtual ~GenerateMazeState();

        virtual void Enter() override;
        virtual void Leave() override;
        virtual State* Tick(double deltaTime) override;
    };

    class FlyMazeInState : public State
    {
    public:
        FlyMazeInState(Game* game);
        virtual ~FlyMazeInState();

        virtual void Enter() override;
        virtual void Leave() override;
        virtual State* Tick(double deltaTime) override;
        virtual double GetTransitionAlpha() const override;

        double animRate;
        double transitionAlpha;
    };

    class FlyMazeOutState : public State
    {
    public:
        FlyMazeOutState(Game* game);
        virtual ~FlyMazeOutState();

        virtual void Enter() override;
        virtual void Leave() override;
        virtual State* Tick(double deltaTime) override;
        virtual double GetTransitionAlpha() const override;

        double animRate;
        double transitionAlpha;
    };

    class PlayGameState : public State
    {
    public:
        PlayGameState(Game* game);
        virtual ~PlayGameState();

        virtual void Enter() override;
        virtual void Leave() override;
        virtual State* Tick(double deltaTime) override;

        int mazeBlockCount;
        int mazeBlockTouchedCount;
    };

    double GetSurfaceAspectRatio() const;

private:

    void SetState(State* newState);

    State* state;
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
    Progress progress;
};