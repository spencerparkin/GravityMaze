#include "MidiManager.h"
#include "AndroidOut.h"
#include <jni.h>

MidiManager::MidiManager(android_app* app)
{
    this->app = app;
    this->midiDevice = nullptr;
    this->midiInputPort = nullptr;
    this->state = State::INITIAL;
    this->stateMethodMap.insert(std::pair<State, StateMethod>(State::INITIAL, &MidiManager::InitialStateHandler));
    this->stateMethodMap.insert(std::pair<State, StateMethod>(State::SHUTDOWN, &MidiManager::ShutdownStateHandler));
    this->stateMethodMap.insert(std::pair<State, StateMethod>(State::DO_NOTHING, &MidiManager::DoNothingStateHandler));
    this->stateMethodMap.insert(std::pair<State, StateMethod>(State::WAIT_FOR_MIDI_DEVICE_OPEN, &MidiManager::WaitForMidiDeviceOpenStateHandler));
    this->stateMethodMap.insert(std::pair<State, StateMethod>(State::PICK_NEW_SONG, &MidiManager::PickNewSongStateHandler));
    this->stateMethodMap.insert(std::pair<State, StateMethod>(State::PLAY_SONG, &MidiManager::PlaySongStateHandler));
    this->stateMethodMap.insert(std::pair<State, StateMethod>(State::PICK_WAIT_TIME_BETWEEN_SONGS, &MidiManager::PickWaitTimeBetweenSongsStateHandler));
    this->stateMethodMap.insert(std::pair<State, StateMethod>(State::WAIT_BETWEEN_SONGS, &MidiManager::WaitBetweenSongsStateHandler));
}

/*virtual*/ MidiManager::~MidiManager()
{
}

void MidiManager::Manage()
{
    StateMethod method = nullptr;
    StateMethodMap::iterator iter = this->stateMethodMap.find(this->state);
    if(iter != this->stateMethodMap.end())
        method = iter->second;
    if(method)
        this->state = (this->*method)();
}

MidiManager::State MidiManager::InitialStateHandler()
{
    // This is how we get at the JNIEnv object, but we never want to detach it.
    // Bad things will happen if you ever detach it from the thread.
    JNIEnv* env = nullptr;
    app->activity->vm->AttachCurrentThread(&env, nullptr);
    if(!env)
        return State::SHUTDOWN;

    jclass clazz = env->GetObjectClass(app->activity->javaGameActivity);
    if(!clazz)
        return State::SHUTDOWN;

    jmethodID method = env->GetMethodID(clazz, "kickOffMidiDeviceOpen", "()Z");
    if(!method)
        return State::SHUTDOWN;

    jboolean result = env->CallBooleanMethod(app->activity->javaGameActivity, method);
    if(result)
    {
        aout << "MIDI device open kicked off successfully!" << std::endl;
        return State::WAIT_FOR_MIDI_DEVICE_OPEN;
    }

    aout << "Failed to kick off MIDI device open." << std::endl;
    return State::SHUTDOWN;
}

MidiManager::State MidiManager::ShutdownStateHandler()
{
    aout << "Shutdown down MIDI stuff..." << std::endl;

    if(this->midiInputPort)
    {
        AMidiInputPort_close(this->midiInputPort);
        this->midiInputPort = nullptr;
    }

    if(this->midiDevice)
    {
        AMidiDevice_release(this->midiDevice);
        this->midiDevice = nullptr;
    }

    return State::DO_NOTHING;
}

MidiManager::State MidiManager::DoNothingStateHandler()
{
    // Indeed.  Do nothin' here.
    return State::DO_NOTHING;
}

MidiManager::State MidiManager::WaitForMidiDeviceOpenStateHandler()
{
    aout << "Waiting for MIDI device open..." << std::endl;

    JNIEnv* env = nullptr;
    app->activity->vm->AttachCurrentThread(&env, nullptr);
    if(!env)
        return State::SHUTDOWN;

    jclass clazz = env->GetObjectClass(app->activity->javaGameActivity);
    if(!clazz)
        return State::SHUTDOWN;

    jmethodID method = env->GetMethodID(clazz, "getOpenedMidiDevice", "()Landroid/media/midi/MidiDevice;");
    if(!method)
        return State::SHUTDOWN;

    jobject object = env->CallObjectMethod(app->activity->javaGameActivity, method);
    if(!object)
    {
        aout << "MIDI device open succeeded!" << std::endl;
        return State::PICK_NEW_SONG;
    }

    if(AMEDIA_OK != AMidiDevice_fromJava(env, object, &this->midiDevice))
        return State::SHUTDOWN;

    if(!this->midiDevice)
        return State::SHUTDOWN;

    // We don't check this, because Android SDK will sometimes mis-report the capabilities of a device.
    //int numInputPorts = AMidiDevice_getNumInputPorts(this->midiDevice);
    //if(numInputPorts == 0)
    //    return State::SHUTDOWN;

    if(AMEDIA_OK != AMidiInputPort_open(this->midiDevice, 0, &this->midiInputPort))
        return State::SHUTDOWN;

    if(!this->midiInputPort)
        return State::SHUTDOWN;

    return State::PICK_NEW_SONG;
}

MidiManager::State MidiManager::PickNewSongStateHandler()
{
    // TODO: Write this.
    return State::PICK_NEW_SONG;
}

MidiManager::State MidiManager::PlaySongStateHandler()
{
    return this->state;
}

MidiManager::State MidiManager::PickWaitTimeBetweenSongsStateHandler()
{
    return this->state;
}

MidiManager::State MidiManager::WaitBetweenSongsStateHandler()
{
    return this->state;
}