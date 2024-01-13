#pragma once

#include <game-activity/native_app_glue/android_native_app_glue.h>
#include <AMidi/AMidi.h>
#include <map>

class MidiManager
{
public:
    MidiManager(android_app* app);
    virtual ~MidiManager();

    void Manage();

private:

    enum State
    {
        INITIAL,
        SHUTDOWN,
        DO_NOTHING,
        WAIT_FOR_MIDI_DEVICE_OPEN,
        PICK_NEW_SONG,
        PLAY_SONG,
        PICK_WAIT_TIME_BETWEEN_SONGS,
        WAIT_BETWEEN_SONGS
    };

    typedef State (MidiManager::* StateMethod)();
    typedef std::map<State, StateMethod> StateMethodMap;

    State InitialStateHandler();
    State ShutdownStateHandler();
    State DoNothingStateHandler();
    State WaitForMidiDeviceOpenStateHandler();
    State PickNewSongStateHandler();
    State PlaySongStateHandler();
    State PickWaitTimeBetweenSongsStateHandler();
    State WaitBetweenSongsStateHandler();

    android_app* app;
    State state;
    StateMethodMap stateMethodMap;
    AMidiDevice* midiDevice;
    AMidiInputPort* midiInputPort;
};