#pragma once

#include <oboe/Oboe.h>

// This is the abstraction layer between our game software and the underlying audio library.
class AudioSubSystem
{
public:
    AudioSubSystem();
    virtual ~AudioSubSystem();

    bool Setup();
    bool Shutdown();

    enum SoundFX
    {
        GOOD_BLOCK_TOUCHED,
        GOOD_BLOCK_RESET,
        ALL_GOOD_BLOCKS_RESET,
        LEVEL_SOLVED,
        EVIL_QUEEN_DESTROYED
    };

    // It's okay to call this even if this sub-system is not initialized; it just won't do anything.
    void PlayFX(SoundFX soundFX);

private:

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
};