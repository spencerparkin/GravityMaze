#include "Game.h"
#include "MazeObject.h"
#include "Math/GeometricAlgebra/Vector2D.h"
#include <game-activity/native_app_glue/android_native_app_glue.h>
#include <GLES3/gl3.h>
#include <memory>
#include <vector>
#include <assert.h>
#include <filesystem>
#include "AndroidOut.h"

using namespace PlanarPhysics;

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
}

/*virtual*/ Game::~Game()
{
}

bool Game::Init()
{
    if(this->initialized)
    {
        aout << "Already initialized game!" << std::endl;
        return false;
    }

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

    this->initialized = true;
    return true;
}

bool Game::Shutdown()
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

void Game::HandleSensorEvent(void* data)
{
    ASensorEvent sensorEvent;
    while(ASensorEventQueue_getEvents(this->sensorEventQueue, &sensorEvent, 1) > 0)
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

void Game::GenerateNextMaze()
{
    this->maze.Clear();

    //::srand(unsigned(time(nullptr)));
    ::srand(0);

    int rows = 6;
    int cols = 3;

    this->maze.Generate(rows, cols);

    this->maze.PopulatePhysicsWorld(&this->physicsEngine);

    this->physicsEngine.accelerationDueToGravity = Vector2D(0.0, -this->options.gravity);
    this->physicsEngine.SetCoefOfRestForAllCHs(this->options.bounce);
}

void Game::Tick()
{
    this->physicsEngine.Tick();
}

void Game::Render()
{
    if(!this->initialized)
        return;

    glClear(GL_COLOR_BUFFER_BIT);

    double aspectRatio = double(this->surfaceWidth) / double(this->surfaceHeight);
    this->drawHelper.BeginRender(&this->physicsEngine, aspectRatio);

    const std::vector<PlanarObject*>& planarObjectArray = this->physicsEngine.GetPlanarObjectArray();
    for(const PlanarObject* planarObject : planarObjectArray)
    {
        auto mazeObject = dynamic_cast<const MazeObject*>(planarObject);
        if(mazeObject)
        {
            mazeObject->Render(this->drawHelper);
        }
    }

    this->drawHelper.EndRender();

    auto swapResult = eglSwapBuffers(this->display, this->surface);
    assert(swapResult == EGL_TRUE);
}