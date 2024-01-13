#include "MidiManager.h"
#include "AndroidOut.h"
#include "MidiFileFormat.h"
#include "MidiData.h"
#include "Error.h"
#include "Math/Utilities/Random.h"
#include <android/asset_manager.h>
#include <jni.h>

using namespace AudioDataLib;

MidiManager::MidiManager(android_app* app) : AudioDataLib::MidiPlayer(nullptr)
{
    this->app = app;
    this->midiDevice = nullptr;
    this->midiInputPort = nullptr;
    this->nextSongOffset = 0;
    this->currentMidiData = nullptr;
    this->waitTimeBetweenSongsSeconds = 0.0;
    this->waitTimeBegin = 0;
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

void MidiManager::Abort()
{
    this->state = State::SHUTDOWN;
    while(this->state != State::DO_NOTHING)
        this->Manage();
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

    Error error;
    this->EndPlayback(error);

    if(this->currentMidiData)
    {
        MidiData::Destroy(this->currentMidiData);
        this->currentMidiData = nullptr;
    }

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
    AAssetManager* assetManager = this->app->activity->assetManager;

    if(this->shuffledSongArray.size() == 0 || this->nextSongOffset >= this->shuffledSongArray.size())
    {
        this->shuffledSongArray.clear();

        AAssetDir* songDir = AAssetManager_openDir(assetManager, "midi_songs");
        if(!songDir)
            return State::SHUTDOWN;

        while(true)
        {
            const char* songFile = AAssetDir_getNextFileName(songDir);
            if(!songDir)
                break;

            this->shuffledSongArray.push_back(songFile);
        }

        AAssetDir_close(songDir);

        if(this->shuffledSongArray.size() == 0)
            return State::SHUTDOWN;

        PlanarPhysics::Random::ShuffleArray<std::string>(this->shuffledSongArray);
        this->nextSongOffset = 0;
    }

    std::string songFile = this->shuffledSongArray[this->nextSongOffset++];
    AAsset* songAsset = AAssetManager_open(assetManager, songFile.c_str(), AASSET_MODE_BUFFER);
    if(!songAsset)
        return State::SHUTDOWN;

    int songAssetSize = AAsset_getLength(songAsset);
    const unsigned char* songAssetBuf = static_cast<const unsigned char*>(AAsset_getBuffer(songAsset));
    Error error;
    FileData* fileData = nullptr;
    ReadOnlyBufferStream bufferStream(songAssetBuf, songAssetSize);
    MidiFileFormat fileFormat;
    bool success = fileFormat.ReadFromStream(bufferStream, fileData, error);
    AAsset_close(songAsset);

    if(!success)
    {
        delete fileData;
        aout << "Failed to read MIDI file: " + songFile << std::endl;
        aout << "Reason: " + error.GetMessage() << std::endl;
        return State::SHUTDOWN;
    }

    this->currentMidiData = dynamic_cast<MidiData*>(fileData);
    if(!this->currentMidiData)
    {
        delete fileData;
        return State::SHUTDOWN;
    }

    this->SetMidiData(this->currentMidiData);

    std::set<uint32_t> tracksToPlaySet;
    this->GetSimultaneouslyPlayableTracks(tracksToPlaySet);
    if(!this->BeginPlayback(tracksToPlaySet, error))
        return State::SHUTDOWN;

    return State::PLAY_SONG;
}

MidiManager::State MidiManager::PlaySongStateHandler()
{
    Error error;

    if(this->NoMoreToPlay())
    {
        this->EndPlayback(error);
        this->SetMidiData(nullptr);
        delete this->currentMidiData;
        this->currentMidiData = nullptr;
        return State::PICK_WAIT_TIME_BETWEEN_SONGS;
    }

    if(!this->ManagePlayback(error))
    {
        aout << "Error occurred during playback management!" << std::endl;
        aout << "Error: " + error.GetMessage() << std::endl;
        return State::SHUTDOWN;
    }

    return State::PLAY_SONG;
}

MidiManager::State MidiManager::PickWaitTimeBetweenSongsStateHandler()
{
    this->waitTimeBetweenSongsSeconds = PlanarPhysics::Random::Number(10.0, 20.0);
    this->waitTimeBegin = ::clock();
    return State::WAIT_BETWEEN_SONGS;
}

MidiManager::State MidiManager::WaitBetweenSongsStateHandler()
{
    clock_t waitTimeNow = ::clock();
    clock_t waitTimeElapsed = waitTimeNow - this->waitTimeBegin;
    double waitTimeElapsedSeconds = double(waitTimeElapsed) / double(CLOCKS_PER_SEC);
    if(waitTimeElapsedSeconds >= this->waitTimeBetweenSongsSeconds)
        return State::PICK_NEW_SONG;

    return State::WAIT_BETWEEN_SONGS;
}

/*virtual*/ bool MidiManager::SendMessage(const uint8_t* message, uint64_t messageSize, AudioDataLib::Error& error)
{
    if(!this->midiInputPort)
    {
        error.Add("No MIDI input port to which we can send the message!");
        return false;
    }

    // Hmmm...not so sure this loop is such a good idea.
    ssize_t bytesRemaining = messageSize;
    ssize_t totalBytesSent = 0;
    while(bytesRemaining > 0)
    {
        ssize_t numBytesSent = AMidiInputPort_send(this->midiInputPort, &message[totalBytesSent], bytesRemaining);
        totalBytesSent += numBytesSent;
        bytesRemaining -= numBytesSent;
    }

    return true;
}