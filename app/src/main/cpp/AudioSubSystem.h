#pragma once

#include <oboe/Oboe.h>
#include <ByteStream.h>
#include <AudioData.h>
#include <WaveFormat.h>
#include <Mutex.h>
#include <android/asset_manager.h>
#include <vector>

// This is the abstraction layer between our game software and the underlying audio library.
class AudioSubSystem
{
public:
    AudioSubSystem();
    virtual ~AudioSubSystem();

    bool Setup(AAssetManager* assetManager);
    bool Shutdown();

    enum SoundFXType
    {
        GOOD_OUTCOME,
        BAD_OUTCOME
    };

    void PlayFX(SoundFXType soundFXType);

private:

    class AudioClip
    {
    public:
        AudioClip();
        virtual ~AudioClip();

        bool Load(const char* audioFilePath, AAssetManager* assetManager);
        void Unload();

        int bitsPerSample;
        int numChannels;
        int numFrames;
        int sampleRate;
        float* waveBuf;
    };

    class AudioFeeder : public oboe::AudioStreamCallback
    {
    public:
        AudioFeeder();
        virtual ~AudioFeeder();

        virtual oboe::DataCallbackResult onAudioReady(oboe::AudioStream* audioStream, void* audioData, int32_t numAudioFrames) override;
    };

    bool systemSetup;
    oboe::AudioStream* audioStream;
    AudioFeeder audioFeeder;
    std::vector<AudioClip*> audioClipArray;
};