#pragma once

#include <EGL/egl.h>
#include <memory>
#include <android/sensor.h>
#include "AudioSubSystem.h"
#include "Engine.h"
#include "Maze.h"
#include "DrawHelper.h"
#include "Options.h"
#include "Progress.h"
#include "TextRenderer.h"

struct android_app;

class MazeQueen;

#define FINAL_GRAVITY_MAZE_LEVEL        40

class Game
{
public:
    Game(android_app* app);
    virtual ~Game();

    bool Setup();
    bool Shutdown();
    bool SetupWindow();
    bool ShutdownWindow();
    void Render();
    bool Tick();
    void HandleSensorEvent(void* data);
    void SaveProgress();

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
        virtual void Render(DrawHelper& drawHelper) const;

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
    };

    class GameWonState : public State
    {
    public:
        GameWonState(Game* game);
        virtual ~GameWonState();

        virtual void Enter() override;
        virtual void Leave() override;
        virtual State* Tick(double deltaTime) override;
        virtual void Render(DrawHelper& drawHelper) const override;
        virtual double GetTransitionAlpha() const override;
    };

    double GetSurfaceAspectRatio() const;

    class PhysicsWorld : public PlanarPhysics::Engine
    {
    public:
        PhysicsWorld();
        virtual ~PhysicsWorld();

        bool IsMazeSolved();
        int GetGoodMazeBlockCount();
        int GetGoodMazeBlockTouchedCount();
        bool QueenDeadOrNonExistent();
        MazeQueen* FindTheQueen();
    };

    static void HandleAndroidCommand(android_app* app, int32_t cmd);

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
    PhysicsWorld physicsWorld;
    Maze maze;
    DrawHelper drawHelper;
    ASensorManager* sensorManager;
    const ASensor* gravitySensor;
    ASensorEventQueue* sensorEventQueue;
    Options options;
    Progress progress;
    TextRenderer textRenderer;
    AudioSubSystem audioSubSystem;
    clock_t lastTime;
    bool debugWinEntireGame;    // This variable is only meant to be changed by an attached debugger.
};