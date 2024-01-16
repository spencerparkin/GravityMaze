#pragma once

#include <pthread.h>
#include "TimeKeeper.h"
#include "Maze.h"
#include "MazeObject.h"
#include "MazeObjects/MazeBlock.h"
#include "MazeObjects/MazeBall.h"
#include "MazeObjects/MazeQueen.h"
#include "Math/GeometricAlgebra/Vector2D.h"
#include "Math/Utilities/Random.h"
#include "PlanarObjects/Wall.h"
#include "PlanarObjects/Ball.h"
#include "PlanarObjects/RigidBody.h"
#include "TextRenderer.h"
#include "Progress.h"

#define FINAL_GRAVITY_MAZE_LEVEL        40

class GameRender;

class GameLogic
{
public:
    GameLogic(GameRender* gameRender);
    virtual ~GameLogic();

    bool Setup();
    bool Shutdown();
    bool Tick();

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

private:
    class State
    {
    public:
        State(GameLogic* game);
        virtual ~State();

        virtual void Enter();
        virtual void Leave();
        virtual State* Tick(double deltaTime);
        virtual double GetTransitionAlpha() const;
        virtual void Render(DrawHelper& drawHelper) const;

        GameLogic* game;
    };

    class GenerateMazeState : public State
    {
    public:
        GenerateMazeState(GameLogic* game);
        virtual ~GenerateMazeState();

        virtual void Enter() override;
        virtual void Leave() override;
        virtual State* Tick(double deltaTime) override;
    };

    class FlyMazeInState : public State
    {
    public:
        FlyMazeInState(GameLogic* game);
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
        FlyMazeOutState(GameLogic* game);
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
        PlayGameState(GameLogic* game);
        virtual ~PlayGameState();

        virtual void Enter() override;
        virtual void Leave() override;
        virtual State* Tick(double deltaTime) override;
    };

    class GameWonState : public State
    {
    public:
        GameWonState(GameLogic* game);
        virtual ~GameWonState();

        virtual void Enter() override;
        virtual void Leave() override;
        virtual State* Tick(double deltaTime) override;
        virtual void Render(DrawHelper& drawHelper) const override;
        virtual double GetTransitionAlpha() const override;
    };

    static void* ThreadEntryPoint(void* arg);
    void ThreadFunc();

    bool keepTicking;
    pthread_t threadHandle;

    void SetState(State* newState);

    State* state;
    PhysicsWorld physicsWorld;
    Maze maze;
    GameRender* gameRender;
    TextRenderer textRenderer;
    Progress progress;
    TimeKeeper timeKeeper;
};