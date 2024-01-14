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
        return State::WAIT_FOR_MIDI_DEVICE_OPEN;

    aout << "MIDI device open succeeded!" << std::endl;

    if(AMEDIA_OK != AMidiDevice_fromJava(env, object, &this->midiDevice))
    {
        aout << "Failed to get MIDI device from Java!" << std::endl;
        return State::SHUTDOWN;
    }

    if(!this->midiDevice)
        return State::SHUTDOWN;

    // We don't check this, because Android SDK will sometimes mis-report the capabilities of a device.
    //int numInputPorts = AMidiDevice_getNumInputPorts(this->midiDevice);
    //if(numInputPorts == 0)
    //    return State::SHUTDOWN;

    int32_t deviceType = AMidiDevice_getType(this->midiDevice);
    switch(deviceType)
    {
        case AMIDI_DEVICE_TYPE_USB:
        {
            aout << "Device type is USB" << std::endl;
            break;
        }
        case AMIDI_DEVICE_TYPE_BLUETOOTH:
        {
            aout << "Device Type is BLUETOOTH" << std::endl;
            break;
        }
        case AMIDI_DEVICE_TYPE_VIRTUAL:
        {
            aout << "Device type is VIRTUAL" << std::endl;
            break;
        }
    }

    if(AMEDIA_OK != AMidiInputPort_open(this->midiDevice, 0, &this->midiInputPort))
    {
        aout << "Failed to open input port on MIDI device!" << std::endl;
        return State::SHUTDOWN;
    }

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
            if(!songFile)
                break;

            this->shuffledSongArray.push_back(songFile);
        }

        AAssetDir_close(songDir);

        if(this->shuffledSongArray.size() == 0)
            return State::SHUTDOWN;

        PlanarPhysics::Random::ShuffleArray<std::string>(this->shuffledSongArray);
        this->nextSongOffset = 0;
    }

    std::string songFile = "midi_songs/" + this->shuffledSongArray[this->nextSongOffset++];
    aout << "Loading song: " + songFile << std::endl;
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

    for(uint32_t i : tracksToPlaySet)
    {
        double trackTimeSeconds = 0.0;
        this->currentMidiData->CalculateTrackLengthInSeconds(i, trackTimeSeconds, error);
        aout << "Track " << i << " is " << trackTimeSeconds << " seconds." << std::endl;
    }

    this->SetTimeSeconds(0.0);
    if(!this->BeginPlayback(tracksToPlaySet, error))
        return State::SHUTDOWN;

    aout << "Song should now play!!!" << std::endl;
    return State::PLAY_SONG;
}

MidiManager::State MidiManager::PlaySongStateHandler()
{
    Error error;

    if(this->NoMoreToPlay())
    {
        aout << "Hit end of song!" << std::endl;
        this->EndPlayback(error);
        this->SetMidiData(nullptr);
        delete this->currentMidiData;
        this->currentMidiData = nullptr;
        return State::PICK_WAIT_TIME_BETWEEN_SONGS;
    }

    // TODO: We actually need to call this on a dedicated thread.  That means that this
    //       whole state handler should do nothing more than monitor a thread.
    //       Of course, we need a way to signal it to exit if necessary in our ::Abort() function.
    if(!this->ManagePlayback(error))
    {
        aout << "Error occurred during playback management!" << std::endl;
        aout << "Error: " + error.GetMessage() << std::endl;
        return State::SHUTDOWN;
    }

    int32_t timeSeconds = ::round(this->GetTimeSeconds());
    static int32_t lastTimeSeconds = 0;
    if(timeSeconds != lastTimeSeconds)
    {
        aout << "Playback at " << timeSeconds << " seconds." << std::endl;
        lastTimeSeconds = timeSeconds;
    }

    return State::PLAY_SONG;
}

MidiManager::State MidiManager::PickWaitTimeBetweenSongsStateHandler()
{
    this->waitTimeBetweenSongsSeconds = PlanarPhysics::Random::Number(10.0, 20.0);
    this->waitTimeBegin = ::clock();
    aout << "Waiting " << this->waitTimeBetweenSongsSeconds << " seconds before playing another song." << std::endl;
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