#include "GameRender.h"
#include <android/window.h>
#include <android/native_activity.h>
#include <GLES3/gl3.h>
#include <memory>
#include <vector>
#include <assert.h>
#include <filesystem>
#include "AndroidOut.h"

using namespace PlanarPhysics;

//------------------------------ GameRender ------------------------------

GameRender::GameRender(android_app* app) : midiManager(app)
{
    app->onAppCmd = &GameRender::HandleAndroidCommand;

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

/*virtual*/ GameRender::~GameRender()
{
}

/*static*/ void GameRender::HandleAndroidCommand(android_app* app, int32_t cmd)
{
    GameRender* gameRender = reinterpret_cast<GameRender*>(app->userData);

    switch (cmd)
    {
        case APP_CMD_INIT_WINDOW:
        {
            if(gameRender && !gameRender->SetupWindow())
                gameRender->ShutdownWindow();

            break;
        }
        case APP_CMD_TERM_WINDOW:
        {
            if(gameRender)
                gameRender->ShutdownWindow();

            break;
        }
        case APP_CMD_DESTROY:
        {
            break;
        }
        default:
        {
            break;
        }
    }
}

/*static*/ bool GameRender::MotionEventFilter(const GameActivityMotionEvent* motionEvent)
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
bool GameRender::Setup()
{
    if(this->initialized)
    {
        aout << "Already initialized game!" << std::endl;
        return false;
    }

    this->app->userData = this;

    android_app_set_motion_event_filter(this->app, &GameRender::MotionEventFilter);

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

    this->initialized = true;
    return true;
}

bool GameRender::Shutdown()
{
    this->midiManager.Abort();

    this->ShutdownWindow();

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

bool GameRender::SetupWindow()
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

bool GameRender::ShutdownWindow()
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

bool GameRender::CanRender()
{
    return this->display != EGL_NO_DISPLAY && this->GetAspectRatio() != 0.0;
}

void GameRender::HandleSensorEvent(void* data)
{
    ASensorEvent sensorEvent;
    if(ASensorEventQueue_getEvents(this->sensorEventQueue, &sensorEvent, 1) > 0)
    {
        switch(sensorEvent.type)
        {
            case ASENSOR_TYPE_GRAVITY:
            {
                //aout << "Gravity sensed: " << sensorEvent.vector.x << ", " << sensorEvent.vector.y << ", " << sensorEvent.vector.z << std::endl;

                Vector2D vector(-sensorEvent.vector.x, -sensorEvent.vector.y);
                if(!vector.Normalize())
                    vector = Vector2D(0.0, 0.0);

                // I suppose the game thread could read this value when it has only been partially written.  Hmmm...
                this->gravityVector = vector * this->options.gravity * ::sqrt(::abs(1.0 - ::abs(sensorEvent.vector.z / 9.8)));
                break;
            }
        }
    }
}

void GameRender::HandleTapEvents()
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

bool GameRender::Tick()
{
    this->timeKeeper.Tick();

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
            case GameRender::SENSOR_EVENT_ID:
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

        this->drawHelper.Render();

        // This call can sometimes block for a long time.
        auto swapResult = eglSwapBuffers(this->display, this->surface);
        assert(swapResult == EGL_TRUE);
    }

    return !this->app->destroyRequested;
}

double GameRender::GetAspectRatio() const
{
    if(this->surfaceHeight == 0)
        return 0.0;

    return double(this->surfaceWidth) / double(this->surfaceHeight);
}