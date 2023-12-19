#include "Game.h"
#include <game-activity/native_app_glue/android_native_app_glue.h>
#include <GLES3/gl3.h>
#include <memory>
#include <vector>
#include <assert.h>
#include <android/imagedecoder.h>
#include "AndroidOut.h"

Game::Game(android_app* app)
{
    this->initialized = false;
    this->app = app;
    this->display = EGL_NO_DISPLAY;
    this->surface = EGL_NO_SURFACE;
    this->context = EGL_NO_CONTEXT;
    this->surfaceWidth = 0;
    this->surfaceHeight = 0;
}

/*virtual*/ Game::~Game()
{
}

bool Game::Init()
{
    if(this->initialized)
        return false;

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

    // This gets updated on our initial render.
    this->surfaceWidth = 0;
    this->surfaceHeight = 0;

    glClearColor(1.0f, 0.0f, 0.0f, 0.0f);

    this->initialized = true;
    return true;
}

bool Game::Shutdown()
{
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

    this->initialized = false;
    return true;
}

void Game::Render()
{
    EGLint currentWidth, currentHeight;
    eglQuerySurface(this->display, this->surface, EGL_WIDTH, &currentWidth);
    eglQuerySurface(this->display, this->surface, EGL_HEIGHT, &surfaceHeight);

    if(this->surfaceWidth != currentWidth || this->surfaceHeight != currentHeight)
    {
        this->surfaceWidth = currentWidth;
        this->surfaceHeight = currentHeight;

        glViewport(0, 0, this->surfaceWidth, this->surfaceHeight);

        // TODO: Update orthographic projection too.
    }

    glClear(GL_COLOR_BUFFER_BIT);

    auto swapResult = eglSwapBuffers(this->display, this->surface);
    assert(swapResult == EGL_TRUE);
}

void Game::HandleInput()
{
    auto* inputBuffer = android_app_swap_input_buffers(this->app);
    if (!inputBuffer)
        return;

    for (int i = 0; i < inputBuffer->motionEventsCount; i++)
    {
        auto& motionEvent = inputBuffer->motionEvents[i];
        auto action = motionEvent.action;

        //...
    }

    android_app_clear_motion_events(inputBuffer);

    for (auto i = 0; i < inputBuffer->keyEventsCount; i++)
    {
        auto& keyEvent = inputBuffer->keyEvents[i];

        //...
    }

    android_app_clear_key_events(inputBuffer);
}