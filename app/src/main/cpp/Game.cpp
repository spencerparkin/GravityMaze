#include "Game.h"
#include "MazeObject.h"
#include "MazeObjects/MazeBlock.h"
#include "MazeObjects/MazeBall.h"
#include "MazeObjects/MazeQueen.h"
#include "Math/GeometricAlgebra/Vector2D.h"
#include "Math/Utilities/Random.h"
#include "PlanarObjects/Wall.h"
#include "PlanarObjects/Ball.h"
#include "PlanarObjects/RigidBody.h"
#include <android/window.h>
#include <android/native_activity.h>
#include <GLES3/gl3.h>
#include <memory>
#include <vector>
#include <assert.h>
#include <filesystem>
#include "AndroidOut.h"

using namespace PlanarPhysics;

//------------------------------ Game ------------------------------

Game::Game(android_app* app) : midiManager(app)
{
    app->onAppCmd = &Game::HandleAndroidCommand;

    this->debugWinEntireGame = false;
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
    this->lastTime = 0;
}

/*virtual*/ Game::~Game()
{
    delete this->state;
}

/*static*/ void Game::HandleAndroidCommand(android_app* app, int32_t cmd)
{
    Game* game = reinterpret_cast<Game*>(app->userData);

    switch (cmd)
    {
        case APP_CMD_INIT_WINDOW:
        {
            if(game && !game->SetupWindow())
                game->ShutdownWindow();

            break;
        }
        case APP_CMD_TERM_WINDOW:
        {
            if(game)
            {
                game->progress.SetTouches(game->physicsWorld.GetGoodMazeBlockTouchedCount());
                game->progress.Save(app);

                game->ShutdownWindow();
            }

            break;
        }
        case APP_CMD_DESTROY:
        {
            if(game)
            {
                game->progress.SetTouches(game->physicsWorld.GetGoodMazeBlockTouchedCount());
                game->progress.Save(app);
            }

            break;
        }
        default:
        {
            break;
        }
    }
}

/*static*/ bool Game::MotionEventFilter(const GameActivityMotionEvent* motionEvent)
{
    int sourceClass = motionEvent->source & AINPUT_SOURCE_CLASS_MASK;
    return sourceClass == AINPUT_SOURCE_CLASS_POINTER || sourceClass == AINPUT_SOURCE_CLASS_JOYSTICK;
}

// TODO: Tunnelling is an issue.  Perhaps the best way to solve it is to binary search for
//       a more accurate time of impact in the physics engine, or to ray-cast from position
//       to position to prevent jumping over a collision.  In any case, we should add a fail-
//       safe here to just put any object back in the maze if it falls out of the physics
//       world entirely.
// Note that we don't setup our window stuff here, because there may not be a window yet.
// Furthermore, the window can come and go while the game is setup.
bool Game::Setup()
{
    if(this->initialized)
    {
        aout << "Already initialized game!" << std::endl;
        return false;
    }

    this->app->userData = this;

    android_app_set_motion_event_filter(this->app, &Game::MotionEventFilter);

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

    if(this->options.audio && !this->audioSubSystem.Setup(this->app->activity->assetManager))
    {
        aout << "Failed to initialize the audio sub-system." << std::endl;
        return false;
    }

    this->SetState(new GenerateMazeState(this));

    this->initialized = true;
    return true;
}

bool Game::Shutdown()
{
    this->SetState(nullptr);

    this->midiManager.Abort();

    this->ShutdownWindow();

    this->maze.Clear();
    this->physicsWorld.Clear();

    if(this->sensorEventQueue)
    {
        if(this->gravitySensor)
            ASensorEventQueue_disableSensor(this->sensorEventQueue, this->gravitySensor);

        ASensorManager_destroyEventQueue(this->sensorManager, this->sensorEventQueue);
        this->sensorEventQueue = nullptr;
    }

    this->sensorManager = nullptr;
    this->gravitySensor = nullptr;

    this->audioSubSystem.Shutdown();

    this->app->userData = nullptr;

    this->initialized = false;
    return true;
}

bool Game::SetupWindow()
{
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

    return true;
}

bool Game::ShutdownWindow()
{
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

                Vector2D gravityVector(-sensorEvent.vector.x, -sensorEvent.vector.y);
                if(!gravityVector.Normalize())
                    gravityVector = Vector2D(0.0, 0.0);

                double gravityAccel = this->options.gravity;
                gravityVector *= gravityAccel * ::sqrt(::abs(1.0 - ::abs(sensorEvent.vector.z / 9.8)));

                this->physicsWorld.accelerationDueToGravity = gravityVector;
                break;
            }
        }
    }
}

void Game::HandleTapEvents()
{
    struct android_input_buffer* inputBuffer = android_app_swap_input_buffers(this->app);
    if (!inputBuffer)
        return;

    for (int i = 0; i < inputBuffer->motionEventsCount; i++)
    {
        GameActivityMotionEvent& motionEvent = inputBuffer->motionEvents[i];

        /*
        int j = (motionEvent.action & AMOTION_EVENT_ACTION_POINTER_INDEX_MASK) >> AMOTION_EVENT_ACTION_POINTER_INDEX_SHIFT;
        GameActivityPointerAxes& pointer = motionEvent.pointers[j];
        float x = GameActivityPointerAxes_getX(&pointer);
        float y = GameActivityPointerAxes_getY(&pointer);
         */

        switch (motionEvent.action & AMOTION_EVENT_ACTION_MASK)
        {
            case AMOTION_EVENT_ACTION_DOWN:
            {
                this->audioSubSystem.PlayFX(AudioSubSystem::SoundFXType::GOOD_OUTCOME);

                break;
            }
        }
    }

    android_app_clear_motion_events(inputBuffer);
}

// TODO: Can we off-load the rendering to its own thread?
//       That's what's slowing MIDI and physics/animation down.
//       Play with this.  If we comment out rendering, does
//       the music play fine while in app?  Okay, I played with
//       this and it is clear to me that it is the render that
//       is slowing everything down.  The the physics, or music
//       or preparing what we want to render or even creating
//       the vertex buffers.  It's the GL-swap buffer call that's
//       killing the performance.  But then why does our FPS seem
//       high most of the time?  It's confusing, but all I know is
//       that maybe it would be worth looking into a dedicated
//       render thread.
bool Game::Tick()
{
    clock_t currentTime = ::clock();
    double frameRate = 0.0;
    double elapsedTime = 0.0;
    if(this->lastTime != 0)
    {
        elapsedTime = double(currentTime - this->lastTime) / double(CLOCKS_PER_SEC);
        frameRate = 1.0 / elapsedTime;
    }
    this->lastTime = currentTime;

    this->HandleTapEvents();

    this->audioSubSystem.PumpAudio();
    this->midiManager.Manage();

    void* data = nullptr;
    int events = 0;
    int id = ALooper_pollOnce(0, nullptr, &events, &data);
    if (id >= 0)
    {
        switch(id)
        {
            case Game::SENSOR_EVENT_ID:
            {
                this->HandleSensorEvent(data);
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

    // Don't advance the physics unless we're also able to render it.
    if(this->display != EGL_NO_DISPLAY)
        this->physicsWorld.Tick();

    if(this->state)
    {
        State* newState = this->state->Tick(elapsedTime);
        if(newState != this->state)
            this->SetState(newState);
    }

    android_input_buffer* inputBuffer = android_app_swap_input_buffers(this->app);
    if (inputBuffer)
    {
        for (int i = 0; i < inputBuffer->keyEventsCount; i++)
        {
            GameActivityKeyEvent& keyEvent = inputBuffer->keyEvents[i];
            if(keyEvent.action == AKEY_EVENT_ACTION_UP && keyEvent.keyCode == AKEYCODE_BACK)
            {
                JNIEnv* env = nullptr;
                this->app->activity->vm->AttachCurrentThread(&env, nullptr);
                if(env)
                {
                    jclass clazz = env->GetObjectClass(this->app->activity->javaGameActivity);
                    jmethodID method = env->GetMethodID(clazz, "gameActivityFinished", "()V");
                    env->CallVoidMethod(this->app->activity->javaGameActivity, method);
                    env->DeleteLocalRef(clazz);
                    //this->app->activity->vm->DetachCurrentThread();   Note: I don't think I'm ever supposed to call this.
                }
            }
        }

        android_app_clear_key_events(inputBuffer);
    }

    // Render a frame if we're initialized and we have a window.
    if(this->initialized && this->display != EGL_NO_DISPLAY)
    {
        glClear(GL_COLOR_BUFFER_BIT);

        double transitionAlpha = this->state ? this->state->GetTransitionAlpha() : 0.0;

        double aspectRatio = double(this->surfaceWidth) / double(this->surfaceHeight);
        this->drawHelper.BeginRender(&this->physicsWorld, aspectRatio);

        const std::vector<PlanarObject*> &planarObjectArray = this->physicsWorld.GetPlanarObjectArray();
        for (const PlanarObject* planarObject: planarObjectArray)
        {
            auto mazeObject = dynamic_cast<const MazeObject*>(planarObject);
            if (mazeObject)
            {
                mazeObject->Render(this->drawHelper, transitionAlpha);
            }
        }

        Transform textTransform;
        textTransform.scale = (this->progress.GetLevel() < 5) ? (MAZE_CELL_SIZE / 4.0) : (MAZE_CELL_SIZE / 2.0);
        char text[64];
        Color textColor;
        const BoundingBox &worldBox = this->physicsWorld.GetWorldBox();

        textTransform.translation = Vector2D(worldBox.min.x, worldBox.max.y);
        sprintf(text, "Level %d", this->progress.GetLevel());
        textColor = Color(1.0, 1.0, 1.0);
        this->textRenderer.RenderText(text, textTransform, textColor, this->drawHelper);

        textTransform.translation = Vector2D(worldBox.min.y, worldBox.min.y - textTransform.scale);
        sprintf(text, "FPS = %06.2f", frameRate);
        textColor = (frameRate < 60) ? ((frameRate < 30) ? Color(1.0, 0.0, 0.0) : Color(1.0, 1.0, 0.0)) : Color(0.0, 1.0, 0.0);
        this->textRenderer.RenderText(text, textTransform, textColor, this->drawHelper);

        this->state->Render(drawHelper);

        this->drawHelper.EndRender();

        auto swapResult = eglSwapBuffers(this->display, this->surface);
        assert(swapResult == EGL_TRUE);
    }

    return !this->app->destroyRequested;
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

/*virtual*/ void Game::State::Render(DrawHelper& drawHelper) const
{
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
}

/*virtual*/ void Game::GenerateMazeState::Leave()
{
}

/*virtual*/ Game::State* Game::GenerateMazeState::Tick(double deltaTime)
{
    if(this->game->display == EGL_NO_DISPLAY)
        return this;

    Maze& maze = this->game->maze;
    Engine& physicsEngine = this->game->physicsWorld;
    Options& options = this->game->options;
    Progress& progress = this->game->progress;

    maze.Clear();

    if(!progress.Load(this->game->app))
    {
        aout << "Failed to load progress!" << std::endl;
        progress.Reset();
    }

    int level = progress.GetLevel();
    int rows = level + 5;
    int cols = (int)::round(double(rows) * this->game->GetSurfaceAspectRatio());

    aout << "Level " << level << " is a maze of size " << rows << " by " << cols << "." << std::endl;

    bool queen = (level == FINAL_GRAVITY_MAZE_LEVEL);
    maze.Generate(rows, cols, progress.GetSeedModifier());
    maze.PopulatePhysicsWorld(&physicsEngine, progress.GetTouches(), queen, options.bounce);

    physicsEngine.accelerationDueToGravity = Vector2D(0.0, -options.gravity);

    return new FlyMazeInState(this->game);
}

//------------------------------ Game::FlyMazeInState ------------------------------

Game::FlyMazeInState::FlyMazeInState(Game* game) : State(game)
{
    this->animRate = 1.0;
    this->transitionAlpha = 0.0;
}

/*virtual*/ Game::FlyMazeInState::~FlyMazeInState()
{
}

/*virtual*/ void Game::FlyMazeInState::Enter()
{
    this->transitionAlpha = 0.0;

    Engine& physicsEngine = this->game->physicsWorld;

    const BoundingBox& worldBox = physicsEngine.GetWorldBox();

    Random::Seed((unsigned)::time(nullptr));

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
    this->animRate = 1.0;
    this->transitionAlpha = 0.0;
}

/*virtual*/ Game::FlyMazeOutState::~FlyMazeOutState()
{
}

/*virtual*/ void Game::FlyMazeOutState::Enter()
{
    Engine& physicsEngine = this->game->physicsWorld;

    const BoundingBox& worldBox = physicsEngine.GetWorldBox();

    Vector2D center = worldBox.Center();

    // TODO: This doesn't work as expected.  Why?
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
    {
        MazeQueen* mazeQueen = this->game->physicsWorld.FindTheQueen();
        if((mazeQueen && !mazeQueen->alive) || this->game->debugWinEntireGame)
            return new GameWonState(this->game);

        return new GenerateMazeState(this->game);
    }

    return this;
}

/*virtual*/ double Game::FlyMazeOutState::GetTransitionAlpha() const
{
    return this->transitionAlpha;
}

//------------------------------ Game::PlayGameState ------------------------------

Game::PlayGameState::PlayGameState(Game* game) : State(game)
{
}

/*virtual*/ Game::PlayGameState::~PlayGameState()
{
}

/*virtual*/ void Game::PlayGameState::Enter()
{
}

/*virtual*/ void Game::PlayGameState::Leave()
{
}

/*virtual*/ Game::State* Game::PlayGameState::Tick(double deltaTime)
{
    if(this->game->physicsWorld.IsMazeSolved() || this->game->debugWinEntireGame)
    {
        MazeQueen* mazeQueen = this->game->physicsWorld.FindTheQueen();
        if(!mazeQueen)
            this->game->progress.SetLevel(this->game->progress.GetLevel() + 1);
        else
            this->game->progress.Reset();

        this->game->progress.SetTouches(0);
        this->game->progress.Save(this->game->app);

        return new FlyMazeOutState(this->game);
    }

    return this;
}

//------------------------------ Game::GameWonState ------------------------------

Game::GameWonState::GameWonState(Game* game) : State(game)
{
}

/*virtual*/ Game::GameWonState::~GameWonState()
{
}

/*virtual*/ void Game::GameWonState::Enter()
{
    Engine& physicsEngine = this->game->physicsWorld;

    const BoundingBox& worldBox = physicsEngine.GetWorldBox();

    const std::vector<PlanarObject*>& planarObjectArray = physicsEngine.GetPlanarObjectArray();
    for(PlanarObject* planarObject : planarObjectArray)
    {
        MazeObject *mazeObject = dynamic_cast<MazeObject *>(planarObject);
        if (mazeObject)
        {
            mazeObject->sourceTransform.Identity();
            mazeObject->targetTransform.Identity();
            mazeObject->targetTransform.translation = 2.0 * worldBox.max;   // Move them all off screen.
        }
    }
}

/*virtual*/ void Game::GameWonState::Leave()
{
}

/*virtual*/ double Game::GameWonState::GetTransitionAlpha() const
{
    return 1.0;
}

/*virtual*/ Game::State* Game::GameWonState::Tick(double deltaTime)
{
    return this;
}

/*virtual*/ void Game::GameWonState::Render(DrawHelper& drawHelper) const
{
    Engine& physicsEngine = this->game->physicsWorld;

    const BoundingBox& worldBox = physicsEngine.GetWorldBox();

    Transform textToWorld;
    textToWorld.scale = 80.0;
    textToWorld.translation = Vector2D(0.0, worldBox.max.y / 2.0);
    this->game->textRenderer.RenderText("YOU WIN!!!", textToWorld, Color(1.0, 1.0, 1.0), drawHelper);

    // TODO: Can the user choose here to add their name to a database of game winners?
    //       Where could I host such a database?  Not for free, certainly, so maybe I won't bother.
}

//------------------------------ Game::PhysicsWorld ------------------------------

Game::PhysicsWorld::PhysicsWorld()
{
}

/*virtual*/ Game::PhysicsWorld::~PhysicsWorld()
{
}

bool Game::PhysicsWorld::IsMazeSolved()
{
    return this->GetGoodMazeBlockCount() == this->GetGoodMazeBlockTouchedCount() && this->QueenDeadOrNonExistent();
}

int Game::PhysicsWorld::GetGoodMazeBlockCount()
{
    int count = 0;
    for(auto planarObject : this->GetPlanarObjectArray())
        if(dynamic_cast<GoodMazeBlock*>(planarObject))
            count++;

    return count;
}

int Game::PhysicsWorld::GetGoodMazeBlockTouchedCount()
{
    int count = 0;

    for(auto planarObject : this->GetPlanarObjectArray())
    {
        auto goodMazeBlock = dynamic_cast<GoodMazeBlock *>(planarObject);
        if (goodMazeBlock && goodMazeBlock->IsTouched())
            count++;
    }

    return count;
}

bool Game::PhysicsWorld::QueenDeadOrNonExistent()
{
    MazeQueen* mazeQueen = this->FindTheQueen();
    return !mazeQueen || !mazeQueen->alive;
}

MazeQueen* Game::PhysicsWorld::FindTheQueen()
{
    for(auto planarObject : this->GetPlanarObjectArray())
    {
        auto mazeQueen = dynamic_cast<MazeQueen *>(planarObject);
        if (mazeQueen)
            return mazeQueen;
    }

    return nullptr;
}