#pragma once

#include <game-activity/native_app_glue/android_native_app_glue.h>
#include <oboe/Oboe.h>
#include <ByteStream.h>
#include <AudioData.h>
#include <WaveFileFormat.h>
#include <Mutex.h>
#include <AudioSink.h>
#include <android/asset_manager.h>
#include <vector>
#include <oboe/AudioStreamCallback.h>
#include <AMidi/AMidi.h>

// This is the abstraction layer between our game software and the underlying audio library.
class AudioSubSystem
{
public:
    AudioSubSystem();
    virtual ~AudioSubSystem();

    bool Setup(AAssetManager* assetManager);
    bool Shutdown();
    bool PumpAudio();
    bool ManageMidi(android_app* app);

    enum class SoundFXType
    {
        UNKNOWN,
        GOOD_OUTCOME,
        BAD_OUTCOME
    };

    void PlayFX(SoundFXType soundFXType);

private:

    class ErrorCallback : public oboe::AudioStreamErrorCallback
    {
    public:
        ErrorCallback();
        virtual ~ErrorCallback();

        virtual bool onError(oboe::AudioStream* audioStream, oboe::Result result) override;
    };

    class AudioMutex : public AudioDataLib::Mutex
    {
    public:
        AudioMutex();
        virtual ~AudioMutex();

        virtual void Lock() override;
        virtual void Unlock() override;

    private:
        mutable pthread_mutex_t mutex;
    };

    class AudioClip
    {
    public:
        AudioClip();
        virtual ~AudioClip();

        bool Load(const char* audioFilePath, AAssetManager* assetManager);
        void Unload();

        AudioDataLib::AudioData* audioData;
        SoundFXType type;
    };

    class AudioFeeder : public oboe::AudioStreamCallback
    {
    public:
        AudioFeeder();
        virtual ~AudioFeeder();

        bool Configure(oboe::AudioStream* audioStream);

        virtual oboe::DataCallbackResult onAudioReady(oboe::AudioStream* audioStream, void* audioData, int32_t numAudioFrames) override;

        AudioDataLib::AudioSink audioSink;
    };

    bool systemSetup;
    oboe::AudioStream* audioStream;
    AudioFeeder* audioFeeder;
    std::vector<AudioClip*> audioClipArray;
    ErrorCallback errorCallback;
    AMidiDevice* midiDevice;
    AMidiInputPort* midiInputPort;
};