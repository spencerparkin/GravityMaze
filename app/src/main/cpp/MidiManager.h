#pragma once

#include <game-activity/native_app_glue/android_native_app_glue.h>
#include <AMidi/AMidi.h>
#include <map>
#include <string>
#include <vector>
#include <pthread.h>
#include "MidiPlayer.h"
#include "MidiData.h"

class MidiManager : public AudioDataLib::MidiPlayer
{
public:
    MidiManager(android_app* app);
    virtual ~MidiManager();

    void Manage();
    void Abort();

    virtual bool SendMessage(const uint8_t* message, uint64_t messageSize, AudioDataLib::Error& error) override;

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

    static void* PlaybackThreadEntryPoint(void* arg);

    void PlaybackThread();

    android_app* app;
    State state;
    StateMethodMap stateMethodMap;
    AMidiDevice* midiDevice;
    AMidiInputPort* midiInputPort;
    std::vector<std::string> shuffledSongArray;
    int nextSongOffset;
    AudioDataLib::MidiData* currentMidiData;
    double waitTimeBetweenSongsSeconds;
    clock_t waitTimeBegin;
    pthread_t playbackThread;
    volatile bool playbackThreadRunning;
};