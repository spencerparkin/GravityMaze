#include "Game.h"
#include "MazeObject.h"
#include "MazeObjects/MazeBlock.h"
#include "MazeObjects/MazeBall.h"
#include "Math/GeometricAlgebra/Vector2D.h"
#include "Math/Utilities/Random.h"
#include "PlanarObjects/Wall.h"
#include "PlanarObjects/Ball.h"
#include "PlanarObjects/RigidBody.h"
#include <game-activity/native_app_glue/android_native_app_glue.h>
#include <android/window.h>
#include <GLES3/gl3.h>
#include <memory>
#include <vector>
#include <assert.h>
#include <filesystem>
#include "AndroidOut.h"

using namespace PlanarPhysics;

//------------------------------ Game ------------------------------

Game::Game(android_app* app)
{
    this->initialized = false;
    this->app = app;
    this->display = EGL_NO_DISPLAY;
    this->surface = EGL_NO_SURFACE;
    this->context = EGL_NO_CONTEXT;
    this->surfaceWidth = 0;
    this->surfaceHeight = 0;
    this->sensorManager = nullptr;
    this->gravitySensor = nullptr;
    this->sensorEventQueue = nullptr;
    this->state = nullptr;
}

/*virtual*/ Game::~Game()
{
    delete this->state;
}

bool Game::Init()
{
    if(this->initialized)
    {
        aout << "Already initialized game!" << std::endl;
        return false;
    }

    // We need to do this, because the game doesn't take any input from swipes or touches.
    // TODO: Why doesn't this work?  :(
    GameActivity_setWindowFlags(this->app->activity, AWINDOW_FLAG_KEEP_SCREEN_ON, 0);

    if(!this->options.Load(this->app))
    {
        aout << "Failed to load options!" << std::endl;
        return false;
    }

    this->sensorManager = ASensorManager_getInstanceForPackage("com.spencer.gravitymaze");
    if(!this->sensorManager)
    {
        aout << "Could not get access to the sensor manager singleton." << std::endl;
        return false;
    }

    ASensorList sensorList;
    int sensorCount = ASensorManager_getSensorList(this->sensorManager, &sensorList);
    if(sensorCount == 0)
    {
        aout << "No sensors found on the device!" << std::endl;
        return false;
    }

    aout << "Sensor dump..." << std::endl;
    for(int i = 0; i < sensorCount; i++)
    {
        const ASensor* sensor = sensorList[i];
        aout << "Sensor #" << i << ": " <<
                ASensor_getName(sensor) << "/" <<
                ASensor_getStringType(sensor) << "/" <<
                ASensor_getReportingMode(sensor) << "/" <<
                ASensor_getVendor(sensor) << std::endl;
    }

    // TODO: Add "<uses-feature android:name="android.hardware.sensor.gyroscope" />" to manifest file in appropriate spot.
    //       Or be able to work with an alternative?
    //       E.g., fall-back on ASENSOR_TYPE_GAME_ROTATION_VECTOR if no gravity sensor found?
    this->gravitySensor = ASensorManager_getDefaultSensorEx(this->sensorManager, ASENSOR_TYPE_GRAVITY, false);
    if(!this->gravitySensor)
    {
        aout << "No game rotation vector sensor found!" << std::endl;
        return false;
    }

    ALooper* looper = ALooper_prepare(ALOOPER_PREPARE_ALLOW_NON_CALLBACKS);
    if(!looper)
    {
        aout << "No looper for current thread?" << std::endl;
        return false;
    }

    this->sensorEventQueue = ASensorManager_createEventQueue(this->sensorManager, looper, SENSOR_EVENT_ID, nullptr, nullptr);
    if(!this->sensorEventQueue)
    {
        aout << "Failed to create sensor event queue!" << std::endl;
        return false;
    }

    if(0 != ASensorEventQueue_enableSensor(this->sensorEventQueue, this->gravitySensor))
    {
        aout << "Failed to enable gravity sensor!" << std::endl;
        return false;
    }

    this->display = eglGetDisplay(EGL_DEFAULT_DISPLAY);
    if(this->display == EGL_NO_DISPLAY)
    {
        aout << "No default display." << std::endl;
        return false;
    }

    if(!eglInitialize(this->display, nullptr, nullptr))
    {
        aout << "Failed to initialize EGL display connection." << std::endl;
        return false;
    }

    constexpr EGLint attribs[] = {
            EGL_RENDERABLE_TYPE, EGL_OPENGL_ES3_BIT,
            EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
            EGL_BLUE_SIZE, 8,
            EGL_GREEN_SIZE, 8,
            EGL_RED_SIZE, 8,
            EGL_DEPTH_SIZE, 24,
            EGL_NONE
    };

    EGLint numConfigs = 0;
    eglChooseConfig(this->display, attribs, nullptr, 0, &numConfigs);

    std::unique_ptr<EGLConfig[]> supportedConfigs(new EGLConfig[numConfigs]);
    eglChooseConfig(this->display, attribs, supportedConfigs.get(), numConfigs, &numConfigs);

    EGLConfig* foundConfig = std::find_if(supportedConfigs.get(), supportedConfigs.get() + numConfigs,
        [this](const EGLConfig& config)
        {
            EGLint red = 0, green = 0, blue = 0, depth = 0;
            if (eglGetConfigAttrib(this->display, config, EGL_RED_SIZE, &red) &&
                eglGetConfigAttrib(this->display, config, EGL_GREEN_SIZE, &green) &&
                eglGetConfigAttrib(this->display, config, EGL_BLUE_SIZE, &blue) &&
                eglGetConfigAttrib(this->display, config, EGL_DEPTH_SIZE, &depth))
            {
                return red == 8 && green == 8 && blue == 8 && depth == 24;
            }
            return false;
        });

    if(!foundConfig)
    {
        aout << "Failed to find desired config." << std::endl;
        return false;
    }

    this->surface = eglCreateWindowSurface(this->display, *foundConfig, this->app->window, nullptr);
    if(this->surface == EGL_NO_SURFACE)
    {
        aout << "Failed to create window surface." << std::endl;
        return false;
    }

    EGLint contextAttribs[] = {EGL_CONTEXT_CLIENT_VERSION, 3, EGL_NONE};
    this->context = eglCreateContext(this->display, *foundConfig, nullptr, contextAttribs);
    if(this->context == EGL_NO_CONTEXT)
    {
        aout << "Failed to create context." << std::endl;
        return false;
    }

    if(!eglMakeCurrent(this->display, this->surface, this->surface, this->context))
    {
        aout << "Failed to bind context with display and surface." << std::endl;
        return false;
    }

    eglQuerySurface(this->display, this->surface, EGL_WIDTH, &this->surfaceWidth);
    eglQuerySurface(this->display, this->surface, EGL_HEIGHT, &this->surfaceHeight);

    glViewport(0, 0, this->surfaceWidth, this->surfaceHeight);

    glClearColor(0.0f, 0.0f, 0.0f, 0.0f);

    if(!this->drawHelper.Setup(this->app->activity->assetManager))
    {
        aout << "Failed to setup draw-helper." << std::endl;
        return false;
    }

    this->SetState(new GenerateMazeState(this));

    this->initialized = true;
    return true;
}

bool Game::Shutdown()
{
    this->SetState(nullptr);

    this->drawHelper.Shutdown();

    if (this->display != EGL_NO_DISPLAY)
    {
        eglMakeCurrent(this->display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);

        if (this->context != EGL_NO_CONTEXT)
        {
            eglDestroyContext(this->display, this->context);
            this->context = EGL_NO_CONTEXT;
        }

        if (this->surface != EGL_NO_SURFACE)
        {
            eglDestroySurface(this->display, this->surface);
            this->surface = EGL_NO_SURFACE;
        }

        eglTerminate(this->display);
        this->display = EGL_NO_DISPLAY;
    }

    this->maze.Clear();
    this->physicsEngine.Clear();

    if(this->sensorEventQueue)
    {
        if(this->gravitySensor)
            ASensorEventQueue_disableSensor(this->sensorEventQueue, this->gravitySensor);

        ASensorManager_destroyEventQueue(this->sensorManager, this->sensorEventQueue);
        this->sensorEventQueue = nullptr;
    }

    this->sensorManager = nullptr;
    this->gravitySensor = nullptr;

    this->initialized = false;
    return true;
}

double Game::GetSurfaceAspectRatio() const
{
    return double(this->surfaceWidth) / double(this->surfaceHeight);
}

void Game::HandleSensorEvent(void* data)
{
    ASensorEvent sensorEvent;
    if(ASensorEventQueue_getEvents(this->sensorEventQueue, &sensorEvent, 1) > 0)
    {
        switch(sensorEvent.type)
        {
            case ASENSOR_TYPE_GRAVITY:
            {
                //aout << "Gravity sensed: " << sensorEvent.vector.x << ", " << sensorEvent.vector.y << ", " << sensorEvent.vector.z << std::endl;

                double gravityAccel = this->options.gravity;
                this->physicsEngine.accelerationDueToGravity = Vector2D(-sensorEvent.vector.x, -sensorEvent.vector.y).Normalized();
                this->physicsEngine.accelerationDueToGravity *= gravityAccel * ::sqrt(::abs(1.0 - ::abs(sensorEvent.vector.z / 9.8)));

                break;
            }
        }
    }
}

void Game::Tick()
{
    // TODO: After profiling our frame-rate, and testing with the following line
    //       enabled and disabled, I have identified it as the culprit for periodic
    //       dips in the frame-rate.  More profiling should be done to pin-point
    //       exactly where in the physics tick we are slowing down.  It's probably
    //       in the collision handling, but we need more information.  Is the slow-
    //       down in the broad-phase handling, or the narrow-phase?  Does the lambda
    //       stuff have an impact?  This is all somewhat reproducable on PC, by the
    //       way, because a debug build runs really slow, but a release build seems
    //       reasonably fast, but I have never understood exactly why that is the case.
    double deltaTime = this->physicsEngine.Tick();

    if(this->state)
    {
        State* newState = this->state->Tick(deltaTime);
        if(newState != this->state)
            this->SetState(newState);
    }
}

void Game::SetState(State* newState)
{
    if(this->state)
    {
        this->state->Leave();
        delete this->state;
    }

    this->state = newState;

    if(this->state)
        this->state->Enter();
}

void Game::Render()
{
    if(!this->initialized)
        return;

    glClear(GL_COLOR_BUFFER_BIT);

    double transitionAlpha = this->state ? this->state->GetTransitionAlpha() : 0.0;

    double aspectRatio = double(this->surfaceWidth) / double(this->surfaceHeight);
    this->drawHelper.BeginRender(&this->physicsEngine, aspectRatio);

    const std::vector<PlanarObject*>& planarObjectArray = this->physicsEngine.GetPlanarObjectArray();
    for(const PlanarObject* planarObject : planarObjectArray)
    {
        auto mazeObject = dynamic_cast<const MazeObject*>(planarObject);
        if(mazeObject)
        {
            mazeObject->Render(this->drawHelper, transitionAlpha);
        }
    }

    this->drawHelper.EndRender();

    auto swapResult = eglSwapBuffers(this->display, this->surface);
    assert(swapResult == EGL_TRUE);
}

//------------------------------ Game::State ------------------------------

Game::State::State(Game* game)
{
    this->game = game;
}

/*virtual*/ Game::State::~State()
{
}

/*virtual*/ void Game::State::Enter()
{
}

/*virtual*/ void Game::State::Leave()
{
}

/*virtual*/ Game::State* Game::State::Tick(double deltaTime)
{
    return this;
}

/*virtual*/ double Game::State::GetTransitionAlpha() const
{
    return 1.0;
}

//------------------------------ Game::GenerateMazeState ------------------------------

Game::GenerateMazeState::GenerateMazeState(Game* game) : State(game)
{
}

/*virtual*/ Game::GenerateMazeState::~GenerateMazeState()
{
}

/*virtual*/ void Game::GenerateMazeState::Enter()
{
    ::srand(unsigned(time(nullptr)));

    Maze& maze = this->game->maze;
    Engine& physicsEngine = this->game->physicsEngine;
    Options& options = this->game->options;
    Progress& progress = this->game->progress;

    maze.Clear();

    if(!progress.Load(this->game->app))
        aout << "Failed to load progress!" << std::endl;

    int level = progress.GetLevel();
    int rows = level + 5;
    int cols = (int)::round(double(rows) * this->game->GetSurfaceAspectRatio());

    aout << "Level " << level << " is a maze of size " << rows << " by " << cols << "." << std::endl;

    maze.Generate(rows, cols);
    maze.PopulatePhysicsWorld(&physicsEngine);

    physicsEngine.accelerationDueToGravity = Vector2D(0.0, -options.gravity);
    physicsEngine.SetCoefOfRest<Wall, Ball>(options.bounce);
    physicsEngine.SetCoefOfRest<Wall, RigidBody>(0.5);
    physicsEngine.SetCoefOfRest<Ball, RigidBody>(options.bounce);
}

/*virtual*/ void Game::GenerateMazeState::Leave()
{
}

/*virtual*/ Game::State* Game::GenerateMazeState::Tick(double deltaTime)
{
    return new FlyMazeInState(this->game);
}

//------------------------------ Game::FlyMazeInState ------------------------------

Game::FlyMazeInState::FlyMazeInState(Game* game) : State(game)
{
    this->animRate = 2.0;
    this->transitionAlpha = 0.0;
}

/*virtual*/ Game::FlyMazeInState::~FlyMazeInState()
{
}

/*virtual*/ void Game::FlyMazeInState::Enter()
{
    this->transitionAlpha = 0.0;

    Engine& physicsEngine = this->game->physicsEngine;

    const BoundingBox& worldBox = physicsEngine.GetWorldBox();

    Vector2D verticalTranslation(0.0, worldBox.Height());
    Vector2D horizontalTranslation(worldBox.Width(), 0.0);
    Vector2D diagATranslation(worldBox.Width(), worldBox.Height());
    Vector2D diagBTranslation(worldBox.Width(), -worldBox.Height());

    std::vector<BoundingBox> outerWorldBoxArray;
    outerWorldBoxArray.push_back(worldBox.Translated(verticalTranslation));
    outerWorldBoxArray.push_back(worldBox.Translated(-verticalTranslation));
    outerWorldBoxArray.push_back(worldBox.Translated(horizontalTranslation));
    outerWorldBoxArray.push_back(worldBox.Translated(-horizontalTranslation));
    outerWorldBoxArray.push_back(worldBox.Translated(diagATranslation));
    outerWorldBoxArray.push_back(worldBox.Translated(-diagATranslation));
    outerWorldBoxArray.push_back(worldBox.Translated(diagBTranslation));
    outerWorldBoxArray.push_back(worldBox.Translated(-diagBTranslation));

    const std::vector<PlanarObject*>& planarObjectArray = physicsEngine.GetPlanarObjectArray();
    for(PlanarObject* planarObject : planarObjectArray)
    {
        MazeObject* mazeObject = dynamic_cast<MazeObject*>(planarObject);
        if(mazeObject)
        {
            int i = Random::Integer(0, outerWorldBoxArray.size() - 1);
            double angle = Random::Number(0.0, 2.0 * PLNR_PHY_PI);
            mazeObject->sourceTransform.Identity();
            mazeObject->sourceTransform.translation = outerWorldBoxArray[i].RandomPoint();
            mazeObject->sourceTransform.rotation = PScalar2D(angle).Exponent();
            mazeObject->targetTransform.Identity();
        }
    }
}

/*virtual*/ void Game::FlyMazeInState::Leave()
{
}

/*virtual*/ Game::State* Game::FlyMazeInState::Tick(double deltaTime)
{
    this->transitionAlpha += this->animRate * deltaTime;
    if(this->transitionAlpha > 1.0)
        return new PlayGameState(this->game);

    return this;
}

/*virtual*/ double Game::FlyMazeInState::GetTransitionAlpha() const
{
    return this->transitionAlpha;
}

//------------------------------ Game::FlyMazeOutState ------------------------------

Game::FlyMazeOutState::FlyMazeOutState(Game* game) : State(game)
{
    this->animRate = 2.0;
    this->transitionAlpha = 0.0;
}

/*virtual*/ Game::FlyMazeOutState::~FlyMazeOutState()
{
}

/*virtual*/ void Game::FlyMazeOutState::Enter()
{
    Engine& physicsEngine = this->game->physicsEngine;

    const BoundingBox& worldBox = physicsEngine.GetWorldBox();

    Vector2D center = worldBox.Center();

    const std::vector<PlanarObject*>& planarObjectArray = physicsEngine.GetPlanarObjectArray();
    for(PlanarObject* planarObject : planarObjectArray)
    {
        MazeObject* mazeObject = dynamic_cast<MazeObject*>(planarObject);
        if(mazeObject)
        {
            mazeObject->sourceTransform.Identity();
            mazeObject->targetTransform.Identity();
            mazeObject->targetTransform.translation = center - mazeObject->GetPosition();
            mazeObject->targetTransform.scale = 0.0;
        }
    }
}

/*virtual*/ void Game::FlyMazeOutState::Leave()
{
}

/*virtual*/ Game::State* Game::FlyMazeOutState::Tick(double deltaTime)
{
    this->transitionAlpha += this->animRate * deltaTime;
    if(this->transitionAlpha > 1.0)
        return new GenerateMazeState(this->game);

    return this;
}

/*virtual*/ double Game::FlyMazeOutState::GetTransitionAlpha() const
{
    return this->transitionAlpha;
}

//------------------------------ Game::PlayGameState ------------------------------

Game::PlayGameState::PlayGameState(Game* game) : State(game)
{
    this->mazeBlockCount = 0;
    this->mazeBlockTouchedCount = 0;
}

/*virtual*/ Game::PlayGameState::~PlayGameState()
{
}

/*virtual*/ void Game::PlayGameState::Enter()
{
    this->mazeBlockCount = 0;
    this->mazeBlockTouchedCount = 0;
    Engine& physicsEngine = this->game->physicsEngine;
    const std::vector<PlanarObject*>& planarObjectArray = physicsEngine.GetPlanarObjectArray();
    for(PlanarObject* planarObject : planarObjectArray)
    {
        auto mazeBlock = dynamic_cast<MazeBlock*>(planarObject);
        if(mazeBlock)
            this->mazeBlockCount++;
    }
}

/*virtual*/ void Game::PlayGameState::Leave()
{
}

/*virtual*/ Game::State* Game::PlayGameState::Tick(double deltaTime)
{
    Engine& physicsEngine = this->game->physicsEngine;
    Engine::CollisionEvent event;
    while(physicsEngine.DequeueCollisionEvent(event))
    {
        MazeBall* mazeBall = nullptr;
        MazeBlock* mazeBlock = nullptr;

        if(event.Cast<MazeBall, MazeBlock>(mazeBall, mazeBlock))
        {
            if(!mazeBlock->touched)
            {
                mazeBlock->touched = true;
                mazeBlock->color = Color(0.0, 0.0, 1.0);
                this->mazeBlockTouchedCount++;
            }
        }
    }

    if(this->mazeBlockTouchedCount == this->mazeBlockCount)
    {
        this->game->progress.SetLevel(this->game->progress.GetLevel() + 1);

        if(!this->game->progress.Save(this->game->app))
            aout << "Failed to save progress!" << std::endl;

        return new FlyMazeOutState(this->game);
    }

    return this;
}